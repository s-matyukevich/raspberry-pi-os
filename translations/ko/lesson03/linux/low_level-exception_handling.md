## 3.2: 리눅스에서 저수준 익셉션 핸들링

거대한 Linux 커널 소스 코드가 주어지면 인터럽트 처리를 담당하는 코드를 찾는 좋은 방법은 무엇일까요? 하나의 아이디어를 제안 할 수 있습니다. 벡터 테이블 기본 주소는 `vbar_el1`레지스터에 저장해야하므로 `vbar_el1`을 검색하면 정확히 벡터 테이블이 초기화되는 위치를 파악할 수 있습니다. 실제로, 검색은 몇 가지 사용법을 제공하며 그중 하나인 [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S)는 이미 우리에게 친숙한 것입니다. 이 코드는 [__primary_switched](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L323) 함수에 있습니다. 이 기능은 MMU가 켜진 후에 실행됩니다. 코드는 다음과 같습니다.

```
    adr_l    x8, vectors            // load VBAR_EL1 with virtual
    msr    vbar_el1, x8            // vector table address
```
이 코드를 통해 벡터 테이블을 `벡터`라고 할 수 있으며 [그 정의](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L367)를 쉽게 찾을 수 있어야합니다.

```
/*
 * Exception vectors.
 */
    .pushsection ".entry.text", "ax"

    .align    11
ENTRY(vectors)
    kernel_ventry    el1_sync_invalid        // Synchronous EL1t
    kernel_ventry    el1_irq_invalid            // IRQ EL1t
    kernel_ventry    el1_fiq_invalid            // FIQ EL1t
    kernel_ventry    el1_error_invalid        // Error EL1t

    kernel_ventry    el1_sync            // Synchronous EL1h
    kernel_ventry    el1_irq                // IRQ EL1h
    kernel_ventry    el1_fiq_invalid            // FIQ EL1h
    kernel_ventry    el1_error_invalid        // Error EL1h

    kernel_ventry    el0_sync            // Synchronous 64-bit EL0
    kernel_ventry    el0_irq                // IRQ 64-bit EL0
    kernel_ventry    el0_fiq_invalid            // FIQ 64-bit EL0
    kernel_ventry    el0_error_invalid        // Error 64-bit EL0

#ifdef CONFIG_COMPAT
    kernel_ventry    el0_sync_compat            // Synchronous 32-bit EL0
    kernel_ventry    el0_irq_compat            // IRQ 32-bit EL0
    kernel_ventry    el0_fiq_invalid_compat        // FIQ 32-bit EL0
    kernel_ventry    el0_error_invalid_compat    // Error 32-bit EL0
#else
    kernel_ventry    el0_sync_invalid        // Synchronous 32-bit EL0
    kernel_ventry    el0_irq_invalid            // IRQ 32-bit EL0
    kernel_ventry    el0_fiq_invalid            // FIQ 32-bit EL0
    kernel_ventry    el0_error_invalid        // Error 32-bit EL0
#endif
END(vectors)
```
익숙하지 않습니까? 실제로이 코드의 대부분을 복사하여 조금 단순화했습니다. [kernel_ventry] (https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L72) 매크로는 거의 RPi OS 안에서 정의된 [ventry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L12)와 같습니다. 그러나 한 가지 차이점은 `kernel_ventry`도 커널 스택 오버플로가 발생했는지 확인하는 역할을 한다는 것입니다. 이 기능은`CONFIG_VMAP_STACK`이 설정되어 있고 `가상적으로 매핑 된 커널 스택`이라는 커널 기능의 일부인 경우에 활성화됩니다. 

### 커널 앤트리

[kernel_entry](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L120) 매크로는 매우 익숙할 것입니다. [corresonding macro] (https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L17)와 RPI OS안에서 동일한 방식으로 사용됩니다. 그러나 원본 (Linux) 버전은 훨씬 더 복잡합니다. 코드는 아래와 같습니다.

```
	.macro	kernel_entry, el, regsize = 64
	.if	\regsize == 32
	mov	w0, w0				// zero upper 32 bits of x0
	.endif
	stp	x0, x1, [sp, #16 * 0]
	stp	x2, x3, [sp, #16 * 1]
	stp	x4, x5, [sp, #16 * 2]
	stp	x6, x7, [sp, #16 * 3]
	stp	x8, x9, [sp, #16 * 4]
	stp	x10, x11, [sp, #16 * 5]
	stp	x12, x13, [sp, #16 * 6]
	stp	x14, x15, [sp, #16 * 7]
	stp	x16, x17, [sp, #16 * 8]
	stp	x18, x19, [sp, #16 * 9]
	stp	x20, x21, [sp, #16 * 10]
	stp	x22, x23, [sp, #16 * 11]
	stp	x24, x25, [sp, #16 * 12]
	stp	x26, x27, [sp, #16 * 13]
	stp	x28, x29, [sp, #16 * 14]

	.if	\el == 0
	mrs	x21, sp_el0
	ldr_this_cpu	tsk, __entry_task, x20	// Ensure MDSCR_EL1.SS is clear,
	ldr	x19, [tsk, #TSK_TI_FLAGS]	// since we can unmask debug
	disable_step_tsk x19, x20		// exceptions when scheduling.

	mov	x29, xzr			// fp pointed to user-space
	.else
	add	x21, sp, #S_FRAME_SIZE
	get_thread_info tsk
	/* Save the task's original addr_limit and set USER_DS (TASK_SIZE_64) */
	ldr	x20, [tsk, #TSK_TI_ADDR_LIMIT]
	str	x20, [sp, #S_ORIG_ADDR_LIMIT]
	mov	x20, #TASK_SIZE_64
	str	x20, [tsk, #TSK_TI_ADDR_LIMIT]
	/* No need to reset PSTATE.UAO, hardware's already set it to 0 for us */
	.endif /* \el == 0 */
	mrs	x22, elr_el1
	mrs	x23, spsr_el1
	stp	lr, x21, [sp, #S_LR]

	/*
	 * In order to be able to dump the contents of struct pt_regs at the
	 * time the exception was taken (in case we attempt to walk the call
	 * stack later), chain it together with the stack frames.
	 */
	.if \el == 0
	stp	xzr, xzr, [sp, #S_STACKFRAME]
	.else
	stp	x29, x22, [sp, #S_STACKFRAME]
	.endif
	add	x29, sp, #S_STACKFRAME

#ifdef CONFIG_ARM64_SW_TTBR0_PAN
	/*
	 * Set the TTBR0 PAN bit in SPSR. When the exception is taken from
	 * EL0, there is no need to check the state of TTBR0_EL1 since
	 * accesses are always enabled.
	 * Note that the meaning of this bit differs from the ARMv8.1 PAN
	 * feature as all TTBR0_EL1 accesses are disabled, not just those to
	 * user mappings.
	 */
alternative_if ARM64_HAS_PAN
	b	1f				// skip TTBR0 PAN
alternative_else_nop_endif

	.if	\el != 0
	mrs	x21, ttbr0_el1
	tst	x21, #0xffff << 48		// Check for the reserved ASID
	orr	x23, x23, #PSR_PAN_BIT		// Set the emulated PAN in the saved SPSR
	b.eq	1f				// TTBR0 access already disabled
	and	x23, x23, #~PSR_PAN_BIT		// Clear the emulated PAN in the saved SPSR
	.endif

	__uaccess_ttbr0_disable x21
1:
#endif

	stp	x22, x23, [sp, #S_PC]

	/* Not in a syscall by default (el0_svc overwrites for real syscall) */
	.if	\el == 0
	mov	w21, #NO_SYSCALL
	str	w21, [sp, #S_SYSCALLNO]
	.endif

	/*
	 * Set sp_el0 to current thread_info.
	 */
	.if	\el == 0
	msr	sp_el0, tsk
	.endif

	/*
	 * Registers that may be useful after this macro is invoked:
	 *
	 * x21 - aborted SP
	 * x22 - aborted PC
	 * x23 - aborted PSTATE
	*/
	.endm
```

이제 우리는`kernel_entry` 매크로를 자세히 살펴볼 것입니다.

```
    .macro    kernel_entry, el, regsize = 64
```

이 매크로는`el`과`regsize`의 두 매개 변수를 허용합니다. `el`은 EL0 또는 EL1에서 예외가 발생했는지에 따라`0` 또는 `1`일 수 있습니다. `regsize`는 32 비트 EL0에서 온 경우 32이고, 그렇지 않으면 64입니다.

```
    .if    \regsize == 32
    mov    w0, w0                // zero upper 32 bits of x0
    .endif
```

32 비트 모드에서는 32 비트 범용 레지스터 (`x0` 대신`w0`)를 사용합니다. `w0`는`x0`의 아랫 부분에 구조적으로 매핑되어 있습니다. 제공된 코드 스니펫은 `w0`을 자체적으로 작성하여 `x0`레지스터의 상위 32 비트를 0으로 만듭니다.

```
    stp    x0, x1, [sp, #16 * 0]
    stp    x2, x3, [sp, #16 * 1]
    stp    x4, x5, [sp, #16 * 2]
    stp    x6, x7, [sp, #16 * 3]
    stp    x8, x9, [sp, #16 * 4]
    stp    x10, x11, [sp, #16 * 5]
    stp    x12, x13, [sp, #16 * 6]
    stp    x14, x15, [sp, #16 * 7]
    stp    x16, x17, [sp, #16 * 8]
    stp    x18, x19, [sp, #16 * 9]
    stp    x20, x21, [sp, #16 * 10]
    stp    x22, x23, [sp, #16 * 11]
    stp    x24, x25, [sp, #16 * 12]
    stp    x26, x27, [sp, #16 * 13]
    stp    x28, x29, [sp, #16 * 14]
```

이 부분은 모든 범용 레지스터를 스택에 저장합니다. 스택 포인터는 [kernel_ventry](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L74)에서 저장되는 것이 필요로한 모든 것에 맞게 이미 조정되어 있어야합니다.
레지스터를 저장하는 순서는 나중에 예외 처리기 내부에 저장된 레지스터에 액세스하는 데 사용되는 특수 구조 [pt_reg](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L119)가 있기 때문에 Linux에서 중요합니다. 보시다시피 이 구조에는 범용 레지스터뿐만 아니라 나중에 `kernel_entry`매크로에서 대부분 채워지는 다른 정보도 포함됩니다. 다음 레슨에서 비슷한 것을 구현하고 사용할 것이기 때문에`pt_regs` 구조체를 기억하는 것이 좋습니다.

```
    .if    \el == 0
    mrs    x21, sp_el0
```

`x21`은 이제 중단 된 스택 포인터를 포함합니다. Linux의 작업은 사용자 및 커널 모드에 대해 서로 다른 2 개의 스택을 사용합니다. 사용자 모드의 경우 `sp_el0`레지스터를 사용하여 예외가 생성 된 순간 스택 포인터 값을 알아낼 수 있습니다. 컨텍스트 전환 중에 스택 포인터를 교체해야하기 때문에이 라인은 매우 중요합니다. 다음 강의에서 자세히 설명하겠습니다.

```
    ldr_this_cpu    tsk, __entry_task, x20    // Ensure MDSCR_EL1.SS is clear,
    ldr    x19, [tsk, #TSK_TI_FLAGS]    // since we can unmask debug
    disable_step_tsk x19, x20        // exceptions when scheduling.
```

`MDSCR_EL1.SS` 비트는 "소프트웨어 단계 예외"를 가능하게합니다. 이 비트가 설정되고 디버그 예외가 마스크 해제되면 명령이 실행 된 후 예외가 생성됩니다. 이것은 일반적으로 디버거에서 사용됩니다. 사용자 모드에서 예외를 처리 할 때는 먼저 [TIF_SINGLESTEP] (https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L93) 플래그를 확인해야합니다. 현재 작업에 설정되어 있습니다. 그렇다면, 이는 작업이 디버거에서 실행 중임을 나타내며`MDSCR_EL1.SS` 비트를 설정 해제해야합니다. 이 코드에서 이해해야 할 중요한 것은 현재 작업에 대한 정보를 얻는 방법입니다. Linux에서 각 프로세스 또는 스레드 (나중에 "작업"으로 참조)에는 [task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L519)가 있습니다. )와 관련이 있습니다. 이 구조체에는 작업에 대한 모든 메타 데이터 정보가 포함됩니다. `arm64` 아키텍처에서`task_struct`는 [thread_info](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L39)라는 다른 구조를 포함합니다. 따라서 `task_struct`에 대한 포인터는 항상 `thread_info`에 대한 포인터로 사용될 수 있습니다. `thread_info`는`entry.S`에 직접 액세스해야하는 다른 저수준 값과 함께 플래그가 저장된 장소입니다.
```
    mov    x29, xzr            // fp pointed to user-space
```
`x29`는 범용 레지스터이지만 일반적으로 특별한 의미가 있습니다. "프레임 포인터"로 사용됩니다. 이제 그 사용 목적을 설명하겠습니다..

함수가 컴파일 될 때, 처음 몇 개의 명령어는 일반적으로 오래된 프레임 포인터와 링크 레지스터 값을 스택에 저장합니다. (단순히 알림 :`x30`은 링크 레지스터라고하며`ret` 명령어가 사용하는 "반환 주소"를 보유합니다.) 그런 다음 새 스택 프레임이 할당되어 함수의 모든 로컬 변수를 포함 할 수 있습니다. 프레임 포인터 레지스터가 프레임의 맨 아래를 가리키도록 설정되어 있습니다. 함수가 로컬 변수에 액세스해야 할 때마다 단순히 하드 코드 된 오프셋을 프레임 포인터에 추가합니다. 오류가 발생하여 스택 추적을 생성해야한다고 상상해보십시오. 현재 프레임 포인터를 사용하여 스택의 모든 로컬 변수를 찾을 수 있으며 링크 레지스터를 사용하여 호출자의 정확한 위치를 파악할 수 있습니다. 다음으로, 우리는 오래된 프레임 포인터와 링크 레지스터 값이 항상 스택 프레임의 시작 부분에 저장된다는 사실을 이용합니다. 호출자의 프레임 포인터를 얻은 후에는 모든 로컬 변수에도 액세스 할 수 있습니다. 이 프로세스는 스택 맨 위에 도달 할 때까지 재귀 적으로 반복되며 "스택 풀기"라고합니다. [ptrace](http://man7.org/linux/man-pages/man2/ptrace.2.html) 시스템 호출에서도 비슷한 알고리즘이 사용됩니다.

이제`kernel_entry` 매크로로 돌아가서, EL0에서 예외를 취한 후 왜 `x29` 레지스터를 지워야하는지 분명해야합니다. Linux에서 각 작업은 사용자 및 커널 모드에 대해 서로 다른 스택을 사용하기 때문에 일반적인 스택 추적을 갖는 것은 의미가 없습니다.

```
    .else
    add    x21, sp, #S_FRAME_SIZE
```

이제 else 절 안에 있습니다. 이는이 코드가 EL1에서 가져온 예외를 처리하는 경우에만 관련이 있음을 의미합니다. 이 경우 우리는 오래된 스택을 재사용하고 제공된 코드 스 니펫은 나중에 사용하기 위해 원래의 `sp`값을 `x21`레지스터에 저장합니다.


```
    /* Save the task's original addr_limit and set USER_DS (TASK_SIZE_64) */
    ldr    x20, [tsk, #TSK_TI_ADDR_LIMIT]
    str    x20, [sp, #S_ORIG_ADDR_LIMIT]
    mov    x20, #TASK_SIZE_64
    str    x20, [tsk, #TSK_TI_ADDR_LIMIT]
```

작업 주소 제한은 사용할 수있는 가장 큰 가상 주소를 지정합니다. 사용자 프로세스가 32 비트 모드에서 작동 할 때이 제한은`2 ^ 32`입니다. 64 비트 커널의 경우 더 클 수 있으며 일반적으로`2 ^ 48`입니다. 32 비트 EL1에서 예외가 발생하면 작업 주소 제한을 [TASK_SIZE_64](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/memory.h#L80)로 변경해야합니다. 또한 실행이 사용자 모드로 돌아 가기 전에 복원해야하므로 원래 주소 제한을 저장해야합니다.

```
    mrs    x22, elr_el1
    mrs    x23, spsr_el1
```

예외 처리를 시작하기 전에`elr_el1`과`spsr_el1`을 스택에 저장해야합니다. RPI OS에서는 아직 수행하지 않았습니다. 현재로서는 예외가 발생한 동일한 위치로 항상 돌아 오기 때문입니다. 그러나 예외를 처리하는 동안 컨텍스트 전환을 수행해야하는 경우 어떻게해야합니까? 다음 강의에서이 시나리오에 대해 자세히 설명하겠습니다.

```
    stp    lr, x21, [sp, #S_LR]
```

링크 레지스터 및 프레임 포인터 레지스터는 스택에 저장됩니다. 우리는 이미 프레임 포인터가 EL0 또는 EL1에서 예외를 가져 왔는지에 따라 다르게 계산되었으며이 계산 결과는 이미`x21` 레지스터에 저장되어 있음을 보았습니다.

```
    /*
     * In order to be able to dump the contents of struct pt_regs at the
     * time the exception was taken (in case we attempt to walk the call
     * stack later), chain it together with the stack frames.
     */
    .if \el == 0
    stp    xzr, xzr, [sp, #S_STACKFRAME]
    .else
    stp    x29, x22, [sp, #S_STACKFRAME]
    .endif
    add    x29, sp, #S_STACKFRAME
```

여기 pt_regs 구조체의 [stackframe](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L140) 속성이 채워집니다. 이 속성에는 링크 레지스터와 프레임 포인터도 포함되어 있지만 이번에는`lr` 대신`elr_el1` (현재 x22) 값이 사용됩니다. `stackframe`은 스택 해제에만 사용됩니다.

```
#ifdef CONFIG_ARM64_SW_TTBR0_PAN
alternative_if ARM64_HAS_PAN
    b    1f                // skip TTBR0 PAN
alternative_else_nop_endif

    .if    \el != 0
    mrs    x21, ttbr0_el1
    tst    x21, #0xffff << 48        // Check for the reserved ASID
    orr    x23, x23, #PSR_PAN_BIT        // Set the emulated PAN in the saved SPSR
    b.eq    1f                // TTBR0 access already disabled
    and    x23, x23, #~PSR_PAN_BIT        // Clear the emulated PAN in the saved SPSR
    .endif

    __uaccess_ttbr0_disable x21
1:
#endif
```

`CONFIG_ARM64_SW_TTBR0_PAN` 매개 변수는 커널이 사용자 공간 메모리에 직접 액세스하지 못하게합니다. 이것이 언제 유용한 지 궁금하다면 [this](https://kernsec.org/wiki/index.php/Exploit_Methods/Userspace_data_usage)를 읽어보십시오. 지금은 이러한 보안 기능이 논의 범위를 벗어 났기 때문에이 기능의 작동 방식에 대한 자세한 설명도 생략하겠습니다.

```
    stp    x22, x23, [sp, #S_PC]
```

`elr_el1`과`spsr_el1`은 스택에 저장됩니다.

```
    /* Not in a syscall by default (el0_svc overwrites for real syscall) */
    .if    \el == 0
    mov    w21, #NO_SYSCALL
    str    w21, [sp, #S_SYSCALLNO]
    .endif
```

`pt_regs` 구조체에는 현재 예외가 시스템 콜인지 아닌지 여부를 나타내는 [field] (https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L132)가 있습니다. 기본적으로 시스템 콜이 아니라고 가정합니다. syscall이 작동하는 방법에 대한 자세한 설명은 5 번 강의까지 기다리십시오.

```
    /*
     * Set sp_el0 to current thread_info.
     */
    .if    \el == 0
    msr    sp_el0, tsk
    .endif
```

커널 모드에서 작업이 실행되면`sp_el0`이 필요하지 않습니다. 그 값은 이전에 스택에 저장되었으므로 `kernel_exit` 매크로로 쉽게 복원 할 수 있습니다. 이 시점부터 시작하여`sp_el0`은 현재 빠른 접속을 위한 [task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L519)에 대한 포인터를 유지하는 데 사용됩니다.

### el1_irq

다음으로 살펴볼 것은 EL1에서 가져온 IRQ를 처리하는 처리기입니다. [vector table](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L374)에서 핸들러가`el1_irq`라는 것을 쉽게 알 수 있습니다. [여기](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L562)에 정의되어 있습니다. 이제 코드를 살펴보고 한 줄씩 살펴 보겠습니다.

```
el1_irq:
    kernel_entry 1
    enable_dbg
#ifdef CONFIG_TRACE_IRQFLAGS
    bl    trace_hardirqs_off
#endif

    irq_handler

#ifdef CONFIG_PREEMPT
    ldr    w24, [tsk, #TSK_TI_PREEMPT]    // get preempt count
    cbnz    w24, 1f                // preempt count != 0
    ldr    x0, [tsk, #TSK_TI_FLAGS]    // get flags
    tbz    x0, #TIF_NEED_RESCHED, 1f    // needs rescheduling?
    bl    el1_preempt
1:
#endif
#ifdef CONFIG_TRACE_IRQFLAGS
    bl    trace_hardirqs_on
#endif
    kernel_exit 1
ENDPROC(el1_irq)
```

이 함수 내에서 다음이 수행됩니다.

*  `kernel_entry` 및`kernel_exit` 매크로는 프로세서 상태를 저장하고 복원하기 위해 호출됩니다. 첫 번째 매개 변수는 EL1에서 예외가 발생했음을 나타냅니다.
* `enable_dbg` 매크로를 호출하면 디버그 인터럽트가 마스크 해제됩니다. 이 시점에서 프로세서 상태가 이미 저장되어 있고 인터럽트 처리기 중간에 디버그 예외가 발생하더라도 올바르게 처리되므로 안전합니다. 처음에 인터럽트 처리 중에 디버그 예외를 마스크 해제해야하는 이유가 궁금하다면 [this](https://github.com/torvalds/linux/commit/2a2830703a2371b47f7b50b1d35cb15dc0e2b717) 커밋 메시지를 읽으십시오.
* `#ifdef CONFIG_TRACE_IRQFLAGS` 블록 내부의 코드는 인터럽트 추적을 담당합니다. 인터럽트 시작 및 종료의 두 가지 이벤트를 기록합니다.
* `#ifdef CONFIG_PREEMPT` 내부의 코드는 현재 작업 플래그에 액세스하여 스케줄러를 호출해야하는지 여부를 확인합니다. 이 코드는 다음 단원에서 자세히 살펴볼 것입니다.
* `irq_handler`-실제 인터럽트 처리가 수행 된 곳입니다.

[irq_handler](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L351)는 매크로이며 다음과 같이 정의됩니다.

```
    .macro    irq_handler
    ldr_l    x1, handle_arch_irq
    mov    x0, sp
    irq_stack_entry
    blr    x1
    irq_stack_exit
    .endm
```

코드에서 볼 수 있듯이`irq_handler`는 [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44) 기능을 실행합니다. 이 기능은 "irq stack"이라고하는 특수 스택으로 실행됩니다. 다른 스택으로 전환해야하는 이유는 무엇입니까? 예를 들어 RPI OS에서는이 작업을 수행하지 않았습니다. 글쎄, 필요하지는 않지만 태스크 스택을 사용하여 인터럽트가 처리되며 인터럽트 핸들러에 얼마나 많은 인터럽트가 남아 있는지 확신 할 수 없습니다.

다음으로, 우리는 [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44)를 보아야합니다. 이것은 함수가 아니고 그러나 변수입니다. [set_handle_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L46) 함수 내에서 설정됩니다. 그러나 누가 그것을 설정 하고 이 시점에 도달 한 후 인터럽트의 페이드는 무엇입니까? 우리는이 단원의 다음 장에서 답을 알아낼 것입니다.

### Conclusion

결론적으로, 우리는 이미 저수준 인터럽트 처리 코드를 살펴보고 벡터 테이블에서`handle_arch_irq`까지의 인터럽트 경로를 추적했다고 말할 수 있습니다. 이것은 인터럽트가 아키텍처 특정 코드를 떠나고 드라이버 코드에 의해 처리되기 시작한 시점입니다. 다음 장의 목표는 드라이버 소스 코드를 통해 타이머 인터럽트의 경로를 추적하는 것입니다.

##### Previous Page

3.1 [Interrupt handling: RPi OS](../../../docs/lesson03/rpi-os.md)

##### Next Page

3.3 [Interrupt handling: Interrupt controllers](../../../docs/lesson03/linux/interrupt_controllers.md)
