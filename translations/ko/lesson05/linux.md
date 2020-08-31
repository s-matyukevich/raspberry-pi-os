## 5.2: User processes and system calls 

RPi OS의 시스템 콜 구현은 리눅스에서 거의 복사해왔기 때문에 이번 챕터에서 설명할 것은 많지 않다. 하지만 리눅스에서 특정 시스템 콜이 어디에서 어떻게 구현되었는지 알 수 있도록 소스코드를 사용할 것이다.

### Creating first user process

먼저 첫 번째 사용자 프로세스가 어떻게 생성되는지에 대해 알아보자. [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) 함수부터 살펴보는 것이 좋겠다. 이 함수는 linux/init/main.c에 있다. 앞에서 살펴본 바와 같이 이 함수는 커널 부팅 과정의 초기에 호출되는 최초의 아키텍처 독립적인 함수이다. 이 함수에서 커널 초기화를 시작하며, 커널 초기화 도중에 첫 번째 사용자 프로세스를 실행하는 것이 이해될 것이다.

실제로 `start_kernel`의 흐름을 따라가다 보면 곧 [kernel_init](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L989) 함수를 발견할 것이다. kernel_init 함수는 다음 코드를 포함한다.

```
    if (!try_to_run_init_process("/sbin/init") ||
        !try_to_run_init_process("/etc/init") ||
        !try_to_run_init_process("/bin/init") ||
        !try_to_run_init_process("/bin/sh"))
        return 0;
```
이것이 우리가 찾던 것인 듯하다. 이 코드로부터 리눅스 커널이 `init` 사용자 프로그램을 어디에서 시작해서 어느 순서로 찾는지를 추론할 수 있다. 그러면 `try_to_run_init_process` 함수는 [execve](http://man7.org/linux/man-pages/man2/execve.2.html)시스템 콜을 처리하기 위해 [do_execve](https://github.com/torvalds/linux/blob/v4.14/fs/exec.c#L1837)함수를 실행한다. 이 시스템 콜은 binary executable file을 읽어서 현재 프로세스에서 실행한다.

`execve` 시스템 콜은 lesson9에서 자세히 살펴볼 것이다. 지금은 이 시스템 콜의 가장 중요한 작업은 executable file을 구문 분석(parsing)하고 해당 내용을 메모리에 로드하는 것이고 이는 [load_elf_binary](https://github.com/torvalds/linux/blob/v4.14/fs/binfmt_elf.c#L679) 함수에서 수행된다. 여기서 우리는 executable file이 [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) 형식이라고 가정한다. `load_elf_binary` 함수의 끝부분에서 아키텍처 별 [start_thread](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L119) 함수가 호출된다. RPi OS에서는 이를 `move_to_user_mode` 루틴의 프로토타입으로 사용했으며, 다음 코드가 우리가 주로 관심 있는 코드이다.
```
static inline void start_thread_common(struct pt_regs *regs, unsigned long pc)
{
    memset(regs, 0, sizeof(*regs));
    forget_syscall(regs);
    regs->pc = pc;
}

static inline void start_thread(struct pt_regs *regs, unsigned long pc,
                unsigned long sp)
{
    start_thread_common(regs, pc);
    regs->pstate = PSR_MODE_EL0t;
    regs->sp = sp;
}
```
`start_thread`가 실행될 때, 현재 프로세스는 커널 모드에서 동작한다.  `start_thread`는 저장된 `pstate`, `sp`, `pc` 필드를 설정하는 데 사용되는 현재 `pt_regs` 구조체에 접근할 수 있다. 이 논리는 RPi OS의 `move_to_user_mode` 함수와 정확히 동일하므로 생략하겠다. 기억해야 할 것은 `start_thread` 함수가 `kernel_exit` 매크로가 사용자 모드로 프로세스를 이동시키는 방식으로 저장된 프로세서 상태를 준비한다는 것이다.

###  Linux syscalls

기본 시스템 콜 메커니즘은 리눅스 및 RPi OS에서 정확히 동일하다. 이제 이미 익숙한 [clone](http://man7.org/linux/man-pages/man2/clone.2.html) 시스템 콜을 사용하여 이 메커니즘의 자세한 부분을 이해해보자. [glibc clone wrapper](https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/unix/sysv/linux/aarch64/clone.S;h=e0653048259dd9a3d3eb3103ec2ae86acb43ef48;hb=HEAD#l35) 함수부터 시작하자. 이 함수는 RPi OS의 [call_sys_clone](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L22)  함수와 동일하게 동작하지만, 이전 함수에서 전달인자의 온전성을 검사하고 적절하게 예외를 처리한다는 점이 다르다. 꼭 이해하고 기억해야 할 것은 두 가지 경우 모두 `svc` 명령어를 사용해 동기적 예외를 생성하고, 시스템 콜 번호는 `x8` 레지스터에 저장하고 모든 전달인자는 `x0`-`x7` 레지스터에 저장하여 전달한다.

다음으로 `clone` 시스템 콜 정의를 살펴보자. 이 코드는 [여기](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2153) 에서 찾을 수 있으며 다음과 같다.

```
SYSCALL_DEFINE5(clone, unsigned long, clone_flags, unsigned long, newsp,
         int __user *, parent_tidptr,
         int __user *, child_tidptr,
         unsigned long, tls)
{
    return _do_fork(clone_flags, newsp, 0, parent_tidptr, child_tidptr, tls);
}
```
[SYSCALL_DEFINE5](https://github.com/torvalds/linux/blob/v4.14/include/trace/syscall.h#L25) 매크로는 5개의 파라미터로 시스템 콜을 정의한다는 의미의 5를 이름에 포함한다. 이 매크로는 새로운 [syscall_metadata](https://github.com/torvalds/linux/blob/v4.14/include/trace/syscall.h#L25) 구조체를 할당한 뒤 값을 채우고, `sys_<syscall name>` 함수를 생성한다. 예를 들어 `clone` 시스템 콜의 경우 `sys_clone` 함수가 정의된다. 이 함수는 실제로 하위 계층 아키텍처 코드로부터 호출되는 시스템 콜 handler이다.

시스템 콜이 실행될 때, 커널은 시스템 콜 번호로 시스템 콜 handler를 찾을 방법이 필요하다. 이를 위한 가장 쉬운 방법은 시스템 콜 handler에 대한 포인터 배열을 생성하여 각 시스템 콜 번호를 배열의 인덱스로 사용하는 것이다. 이러한 접근 방식을 RPi OS에서 사용했으며 리눅스에서도 동일한 접근 방식을 사용한다. 이 배열을 [sys_call_table](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/sys.c#L62) 이라고 하며 다음과 같이 정의된다.

```
void * const sys_call_table[__NR_syscalls] __aligned(4096) = {
    [0 ... __NR_syscalls - 1] = sys_ni_syscall,
#include <asm/unistd.h>
};
``` 

모든 시스템 콜은 초기에 `sys_ni_syscall` 함수를 가리키도록 할당된다. 여기서 ni는 존재하지 않음을 의미한다. `0`번 시스템 콜과 현재 아키텍처에 대해 정의되지 않은 모든 시스템 콜은 이 handler를 유지한다. `sys_call_table` 배열에 있는 다른 모든 handler는 [asm/unistd.h](https://github.com/torvalds/linux/blob/v4.14/include/uapi/asm-generic/unistd.h) 헤더파일에 다시 쓰여진다. 이 파일은 간단히 시스템 콜 번호와 핸들러 함수 간의 맵핑을 제공한다.

### Low-level syscall handling code

`sys_call_table`이 어떻게 생성되고 채워지는지를 보았다. 이제 하위 계층 시스템 콜 처리 코드에서 어떻게 사용되는지 알아보자. 다시 말하지만 기본 메커니즘은 RPi OS와 거의 동일하다.

일부 시스템 콜은 동기적 예외이며 모든 exception handler는 exception vector table에 정의되어 있다는 것을 알고 있다. 우리가 관심 있는 handler는 EL0에서 생성된 동기적 예외를 처리하는 것이다. 각 예외가 올바른 handler를 찾도록 하는 것을 [el0_sync](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L598)라고 한다.
```
el0_sync:
    kernel_entry 0
    mrs    x25, esr_el1            // read the syndrome register
    lsr    x24, x25, #ESR_ELx_EC_SHIFT    // exception class
    cmp    x24, #ESR_ELx_EC_SVC64        // SVC in 64-bit state
    b.eq    el0_svc
    cmp    x24, #ESR_ELx_EC_DABT_LOW    // data abort in EL0
    b.eq    el0_da
    cmp    x24, #ESR_ELx_EC_IABT_LOW    // instruction abort in EL0
    b.eq    el0_ia
    cmp    x24, #ESR_ELx_EC_FP_ASIMD    // FP/ASIMD access
    b.eq    el0_fpsimd_acc
    cmp    x24, #ESR_ELx_EC_FP_EXC64    // FP/ASIMD exception
    b.eq    el0_fpsimd_exc
    cmp    x24, #ESR_ELx_EC_SYS64        // configurable trap
    b.eq    el0_sys
    cmp    x24, #ESR_ELx_EC_SP_ALIGN    // stack alignment exception
    b.eq    el0_sp_pc
    cmp    x24, #ESR_ELx_EC_PC_ALIGN    // pc alignment exception
    b.eq    el0_sp_pc
    cmp    x24, #ESR_ELx_EC_UNKNOWN    // unknown exception in EL0
    b.eq    el0_undef
    cmp    x24, #ESR_ELx_EC_BREAKPT_LOW    // debug exception in EL0
    b.ge    el0_dbg
    b    el0_inv
```

여기서 현재 예외가 시스템 콜인지 아닌지 알기 위해 `esr_el1`레지스터가 사용된다. 시스템 콜인 경우 [el0_svc](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L837) 함수가 호출된다. 이 함수에 대해 살펴보자.

```
el0_svc:
    adrp    stbl, sys_call_table        // load syscall table pointer
    mov    wscno, w8            // syscall number in w8
    mov    wsc_nr, #__NR_syscalls
el0_svc_naked:                    // compat entry point
    stp    x0, xscno, [sp, #S_ORIG_X0]    // save the original x0 and syscall number
    enable_dbg_and_irq
    ct_user_exit 1

    ldr    x16, [tsk, #TSK_TI_FLAGS]    // check for syscall hooks
    tst    x16, #_TIF_SYSCALL_WORK
    b.ne    __sys_trace
    cmp     wscno, wsc_nr            // check upper syscall limit
    b.hs    ni_sys
    ldr    x16, [stbl, xscno, lsl #3]    // address in the syscall table
    blr    x16                // call sys_* routine
    b    ret_fast_syscall
ni_sys:
    mov    x0, sp
    bl    do_ni_syscall
    b    ret_fast_syscall
ENDPROC(el0_svc)
```

Now, let's examine it line by line.

```
el0_svc:
    adrp    stbl, sys_call_table        // load syscall table pointer
    mov    wscno, w8            // syscall number in w8
    mov    wsc_nr, #__NR_syscalls
```

처음 세 줄에서는 어떤 레지스터의 별칭인 `stbl`, `wscno`, `wsc_nr` 변수가 초기화된다. `stbl`은 시스템 콜 테이블의 주소를 가지고, `wsc_nr`은 총 시스템 콜 개수를 가지며, `wscno`는 `x8` 레지스터에 저장된 현재 시스템 콜 번호를 뜻한다.
```
    stp    x0, xscno, [sp, #S_ORIG_X0]    // save the original x0 and syscall number
```
RPi OS에서 다루었듯이 시스템 콜이 종료된 후 `x0`은 `pt_regs` 영역에 덮여씌워진다. `x0` 레지스터의 기존 값이 필요한 경우, `pt_regs`구조체의 별도 필드에 저장된다. 마찬가지로 시스템 콜 번호도 `pt_regs`에 저장된다.
```
    enable_dbg_and_irq
```

인터럽트와 debug exception을 활성화한다.

```
    ct_user_exit 1
``` 

사용자 모드에서 커널 모드로의 전환하는 것을 기록한다.

```
    ldr    x16, [tsk, #TSK_TI_FLAGS]    // check for syscall hooks
    tst    x16, #_TIF_SYSCALL_WORK
    b.ne    __sys_trace

```

현재 task가 시스템 콜 추적 프로그램에서 실행되는 경우에 `_TIF_SYSCALL_WORK` 플래그가 set되어야 한다. 이 경우 `__sys_trace` 함수가 호출된다. 일반적인 경우만 살펴보기 위해 이 함수는 넘어가겠다.

```
    cmp     wscno, wsc_nr            // check upper syscall limit
    b.hs    ni_sys
```

현재 시스템 콜 번호가 총 시스템 콜 개수보다 큰 경우, 사용자에게 오류를 리턴한다.

```
    ldr    x16, [stbl, xscno, lsl #3]    // address in the syscall table
    blr    x16                // call sys_* routine
    b    ret_fast_syscall
```

시스템 콜 번호는 시스템 콜 테이블 배열에서 handler를 찾기위한 인덱스로 사용된다. Handler 주소는 `x16` 레지스터에 로드되어 실행된다. 마침내 `ret_fast_syscall` 함수에 이르렀다.

```
ret_fast_syscall:
    disable_irq                // disable interrupts
    str    x0, [sp, #S_X0]            // returned x0
    ldr    x1, [tsk, #TSK_TI_FLAGS]    // re-check for syscall tracing
    and    x2, x1, #_TIF_SYSCALL_WORK
    cbnz    x2, ret_fast_syscall_trace
    and    x2, x1, #_TIF_WORK_MASK
    cbnz    x2, work_pending
    enable_step_tsk x1, x2
    kernel_exit 0
```

중요한 것은 첫 번째 줄에서 인터럽트가 비활성화되고 마지막 줄에서 `kernel_exit` 매크로가 호출된다는 것이다. 그 밖의 모든 것은 특수한 경우의 처리와 관련이 있다. 따라서 이 함수는 시스템 콜이 실제로 종료되고 사용자 프로세스로 실행이 옮겨지는 곳이다.

### Conclusion

이제 시스템 콜을 생성하고 처리하는 것을 다 살펴보았다. 이 과정은 상대적으로 간단하지만 운영체제에서 필수적이다. 왜냐하면 커널이 API를 설정하고, API가 사용자 프로그램과 커널 간의 유일한 통신 수단임을 확인하기 때문이다.

##### Previous Page

5.1 [User processes and system calls: RPi OS](../../docs/lesson05/rpi-os.md)

##### Next Page

5.3 [User processes and system calls: Exercises](../../docs/lesson05/exercises.md)