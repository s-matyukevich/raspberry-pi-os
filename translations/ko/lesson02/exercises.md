
## 2.2: 프로세서 초기화 (Linux)

`arm64` 아키텍처의 진입 점 인 [stext](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L116) 함수에서 Linux 커널 탐색을 중단했습니다. 이번에는 조금 더 깊이 들어가 이 수업과 이전 수업에서 이미 구현한 코드와 몇 가지 유사점을 찾아 보겠습니다. 이 장은 다른 ARM 시스템 레지스터와 Linux 커널에서 사용되는 방법에 대해 주로 설명하기 때문에 약간 지루할 수 있습니다. 그러나  다음과 같은 이유로 저는 여전히 이 내용이 매우 중요하다고 생각합니다.

1. 하드웨어가 소프트웨어에 제공하는 인터페이스를 이해할 필요가 있습니다. 이 인터페이스를 아는 것만으로도 많은 경우의 특정 커널 기능이 구현되는 방법과 이 기능을 구현하기 위해 소프트웨어와 하드웨어가 어떻게 협력하는지를 분석할 수 있을 것입니다.
1. 시스템 레지스터의 다른 옵션은 일반적으로 다양한 하드웨어 기능을 활성화 / 비활성화하는 것과 관련이 있습니다.  ARM 프로세서에 어떤 시스템 레지스터가 있는지 알고 있다면 어떤 종류의 기능을 지원하는지 이미 알고있을 것입니다.

자, 이제 `stext` 함수의 기능을 다시 살펴 봅시다.

```
ENTRY(stext)
    bl    preserve_boot_args
    bl    el2_setup            // Drop to EL1, w0=cpu_boot_mode
    adrp    x23, __PHYS_OFFSET
    and    x23, x23, MIN_KIMG_ALIGN - 1    // KASLR offset, defaults to 0
    bl    set_cpu_boot_mode_flag
    bl    __create_page_tables
    /*
     * The following calls CPU setup code, see arch/arm64/mm/proc.S for
     * details.
     * On return, the CPU will be ready for the MMU to be turned on and
     * the TCR will have been set.
     */
    bl    __cpu_setup            // initialise processor
    b    __primary_switch
ENDPROC(stext)
``` 

### preserve_boot_args (부팅 인수 유지)

[preserve_boot_args](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L136) 함수는 부트 로더가 커널에 전달한 매개 변수 저장을 담당합니다.

```
preserve_boot_args:
    mov    x21, x0                // x21=FDT

    adr_l    x0, boot_args            // record the contents of
    stp    x21, x1, [x0]            // x0 .. x3 at kernel entry
    stp    x2, x3, [x0, #16]

    dmb    sy                // needed before dc ivac with
                        // MMU off

    mov    x1, #0x20            // 4 x 8 bytes
    b    __inval_dcache_area        // tail call
ENDPROC(preserve_boot_args)
```

[커널 부트 프로토콜](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt#L150)에 따라 매개 변수는 레지스터 `x0-x3`의 커널로 전달됩니다. `x0`에는 시스템 RAM에있는 Device Tree BLOB (`.dtb`)의 물리적 주소가 포함됩니다. `x1-x3`은 향후 사용을 위해 예약되어 있습니다. 이 기능은 x0-x3 레지스터의 내용을 [boot_args](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/setup.c#L93) 배열에 복사 한 다음 데이터 캐시에서 해당 캐시 라인을 [무효화](https://developer.arm.com/docs/den0024/latest/caches/cache-maintenance)합니다. 멀티프로세서 시스템의 캐시 유지보수는 그 자체로 큰 주제인데, 우리는 일단 이 주제를 생략할 것입니다. 이 주제에 관심이 있는 사람들을 위해, `ARM 프로그래머 가이드`의 [캐쉬](https://developer.arm.com/docs/den0024/latest/caches)와 [멀티 코어 프로세서](https://developer.arm.com/docs/den0024/latest/multi-core-processors) 챕터를 읽는 것을 추천하겠습니다. 

### el2_setup (el2 설정)

[arm64boot protocol](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt#L159) 프로토콜에 따라 커널은 EL1 또는 EL2로 부팅 될 수 있습니다. 두 번째 경우(EL2), 커널은 가상화 확장에 액세스 할 수 있으며 호스트 운영 체제로 작동 할 수 있습니다. EL2에서 부팅될 정도로 운이 좋다면 [el2_setup](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L386) 함수가 호출됩니다. EL2에서만 액세스 할 수있는 다른 매개 변수 구성과 EL1 의 삭제를 담당합니다. 이제 이 기능을 작은 부분으로 나누고 각 부분을 하나씩 설명하겠습니다.

```
    msr    SPsel, #1            // We want to use SP_EL{1,2}
``` 

특수 목적 스택 포인터는 EL1 및 EL2 모두에 사용됩니다. 다른 옵션은 EL0의 스택 포인터를 재사용하는 것입니다.

```
    mrs    x0, CurrentEL
    cmp    x0, #CurrentEL_EL2
    b.eq    1f
```

현재 EL이 레이블 `1`에  EL2 분기 인 경우에만 EL2 설정 할 수 있고 그렇지 않으면 EL2 설정을 수행 할 수 없으며 이 기능에서 수행 할 작업이 많지 않습니다.

```
    mrs    x0, sctlr_el1
CPU_BE(    orr    x0, x0, #(3 << 24)    )    // Set the EE and E0E bits for EL1
CPU_LE(    bic    x0, x0, #(3 << 24)    )    // Clear the EE and E0E bits for EL1
    msr    sctlr_el1, x0
    mov    w0, #BOOT_CPU_MODE_EL1        // This cpu booted in EL1
    isb
    ret
```

EL1에서 실행되면 `sctlr_el1` 레지스터가 업데이트되어 [CPU_BIG_ENDIAN](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/Kconfig#L612) 구성 설정 값에 따라 CPU가 `little-endian` 모드의 `big-endian`에서 작동하도록 합니다. 그런 다음 `el2_setup` 함수를 종료하고 [BOOT_CPU_MODE_EL1](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/virt.h#L55)  상수를 반환합니다.
따라서 [ARM64 함수 호출 규칙](http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055b/IHI0055B_aapcs64.pdf) 에 따라 반환 값은 `x0` 레지스터 (또는이 경우` w0`)에 있어야합니다. `w0` 레지스터는 `x0`의 첫 번째 32 비트로 생각할 수 있습니다.

```
1:    mrs    x0, sctlr_el2
CPU_BE(    orr    x0, x0, #(1 << 25)    )    // Set the EE bit for EL2
CPU_LE(    bic    x0, x0, #(1 << 25)    )    // Clear the EE bit for EL2
    msr    sctlr_el2, x0
```

EL2에서 부팅 된 것으로 보이면 EL2에 대해 동일한 종류의 설정을 수행하는 것입니다 (이번에는`sctlr_el1` 대신`sctlr_el2` 레지스터가 사용됩니다).

```
#ifdef CONFIG_ARM64_VHE
    /*
     * Check for VHE being present. For the rest of the EL2 setup,
     * x2 being non-zero indicates that we do have VHE, and that the
     * kernel is intended to run at EL2.
     */
    mrs    x2, id_aa64mmfr1_el1
    ubfx    x2, x2, #8, #4
#else
    mov    x2, xzr
#endif
```

[ARM64_VHE](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/Kconfig#L926)구성 변수를 통해 [VHE (Virtualization Host Extensions)](https://developer.arm.com/products/architecture/a-profile/docs/100942/latest/aarch64-virtualization)를 활성화하고 호스트 시스템이 이를 지원하면 `x2`가 0이 아닌 값으로 업데이트됩니다. `x2`는 나중에 같은 기능에서 `VHE`가 활성화되어 있는지 확인하는 데 사용됩니다.

```
    mov    x0, #HCR_RW            // 64-bit EL1
    cbz    x2, set_hcr
    orr    x0, x0, #HCR_TGE        // Enable Host Extensions
    orr    x0, x0, #HCR_E2H
set_hcr:
    msr    hcr_el2, x0
    isb
```

여기서 `hcr_el2` 레지스터를 설정합니다. 동일한 레지스터를 사용하여 RPi OS에서 EL1에 대한 64 비트 실행 모드를 설정했습니다. 이것은 제공된 코드 샘플의 첫 번째 줄에서 수행되는 작업입니다.  또한`x2! = 0` 인 경우 VHE를 사용할 수 있고 커널이 이를 사용하도록 구성되어 있으면`hcr_el2`도 VHE를 활성화하는 데 사용됩니다.

```
    /*
     * Allow Non-secure EL1 and EL0 to access physical timer and counter.
     * This is not necessary for VHE, since the host kernel runs in EL2,
     * and EL0 accesses are configured in the later stage of boot process.
     * Note that when HCR_EL2.E2H == 1, CNTHCTL_EL2 has the same bit layout
     * as CNTKCTL_EL1, and CNTKCTL_EL1 accessing instructions are redefined
     * to access CNTHCTL_EL2. This allows the kernel designed to run at EL1
     * to transparently mess with the EL0 bits via CNTKCTL_EL1 access in
     * EL2.
     */
    cbnz    x2, 1f
    mrs    x0, cnthctl_el2
    orr    x0, x0, #3            // Enable EL1 physical timers
    msr    cnthctl_el2, x0
1:
    msr    cntvoff_el2, xzr        // Clear virtual offset

```

다음 코드는 위의 주석에 잘 설명되어 있습니다. 추가 할 것이 없습니다.

```
#ifdef CONFIG_ARM_GIC_V3
    /* GICv3 system register access */
    mrs    x0, id_aa64pfr0_el1
    ubfx    x0, x0, #24, #4
    cmp    x0, #1
    b.ne    3f

    mrs_s    x0, SYS_ICC_SRE_EL2
    orr    x0, x0, #ICC_SRE_EL2_SRE    // Set ICC_SRE_EL2.SRE==1
    orr    x0, x0, #ICC_SRE_EL2_ENABLE    // Set ICC_SRE_EL2.Enable==1
    msr_s    SYS_ICC_SRE_EL2, x0
    isb                    // Make sure SRE is now set
    mrs_s    x0, SYS_ICC_SRE_EL2        // Read SRE back,
    tbz    x0, #0, 3f            // and check that it sticks
    msr_s    SYS_ICH_HCR_EL2, xzr        // Reset ICC_HCR_EL2 to defaults

3:
#endif
```

다음 코드 일부는 GICv3을 사용할 수 있고 활성화 된 경우에만 실행됩니다. GIC는 Generic Interrupt Controller의 약자입니다. GIC 사양의 v3 버전은 가상화 컨텍스트에서 특히 유용한 몇 가지 기능을 추가합니다. 예를 들어, GICv3을 사용하면 LPI (Locality-specific Peripheral Interrupt)가 가능합니다. 이러한 인터럽트는 메시지 버스를 통해 라우팅되며 구성은 메모리의 특수 테이블에 유지됩니다.

제공된 코드는 SRE (시스템 레지스터 인터페이스)를 활성화합니다.이 단계는 `ICC_*_ELn` 레지스터를 사용하고 GICv3 기능을 활용하기 전에 수행해야합니다.

```
    /* Populate ID registers. */
    mrs    x0, midr_el1
    mrs    x1, mpidr_el1
    msr    vpidr_el2, x0
    msr    vmpidr_el2, x1
```

`midr_el1` 및`mpidr_el1`은 식별 레지스터 그룹의 읽기 전용 레지스터입니다. 프로세서 제조업체, 프로세서 아키텍처 이름, 코어 수 및 기타 정보에 대한 다양한 정보를 제공합니다. EL1에서 액세스하려는 모든 독자(reader)를 위해 이 정보를 변경할 수 있습니다. 여기서 우리는`vpidr_el2`와`vmpidr_el2`를`midr_el1`과`mpidr_el1`에서 가져온 값으로 채웁니다. 따라서 이 정보는 EL1 또는 높은 예외 레벨에서 액세스하려고 하더라도 동일합니다.

```
#ifdef CONFIG_COMPAT
    msr    hstr_el2, xzr            // Disable CP15 traps to EL2
#endif
```

프로세서가 32 비트 실행 모드에서 실행될 때는 "coprocessor"라는 개념이 있습니다. 64비트 실행 모드에서 일반적으로 시스템 레지스터를 통해 액세스되는 정보에 Coprocessor를 사용할 수 있습니다.  [공식 문서](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0311d/I1014521.html)에서 coprocessor를 통해 정확히 액세스 할 수있는 내용을 읽을 수 있습니다. `msr hstr_el2, xzr` 명령어를 사용하면 낮은 예외 수준의 보조 프로세서를 사용할 수 있습니다. 호환성 모드가 활성화 된 경우에만 수행하는 것이 좋습니다 (이 모드에서 커널은 64 비트 커널에서 32 비트 사용자 응용 프로그램을 실행할 수 있음).

```
    /* EL2 debug */
    mrs    x1, id_aa64dfr0_el1        // Check ID_AA64DFR0_EL1 PMUVer
    sbfx    x0, x1, #8, #4
    cmp    x0, #1
    b.lt    4f                // Skip if no PMU present
    mrs    x0, pmcr_el0            // Disable debug access traps
    ubfx    x0, x0, #11, #5            // to EL2 and allow access to
4:
    csel    x3, xzr, x0, lt            // all PMU counters from EL1

    /* Statistical profiling */
    ubfx    x0, x1, #32, #4            // Check ID_AA64DFR0_EL1 PMSVer
    cbz    x0, 6f                // Skip if SPE not present
    cbnz    x2, 5f                // VHE?
    mov    x1, #(MDCR_EL2_E2PB_MASK << MDCR_EL2_E2PB_SHIFT)
    orr    x3, x3, x1            // If we don't have VHE, then
    b    6f                // use EL1&0 translation.
5:                        // For VHE, use EL2 translation
    orr    x3, x3, #MDCR_EL2_TPMS        // and disable access from EL1
6:
    msr    mdcr_el2, x3            // Configure debug traps
```

이 코드는 `mdcr_el2` (EL2 (Monitor Debug Configuration Register))를 구성합니다. 이 레지스터는 가상화 확장과 관련된 다른 디버그 트랩을 설정합니다. 디버그 및 추적이 논의 범위를 벗어나기 때문에 이 코드 블록의 세부 사항을 설명 하지 않겠습니다.. 자세한 내용은 [AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/ca/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile) 의`2810` page의 `mdcr_el2` 레지스터에 대한 설명을 읽는 것이 좋습니다.

```
    /* Stage-2 translation */
    msr    vttbr_el2, xzr
```

OS를 하이퍼 바이저로 사용하는 경우 게스트 OS에 대한 완벽한 메모리 격리를 제공해야합니다. 2 단계 가상 메모리 변환은이 용도로 정확하게 사용됩니다. 실제로 각 게스트 OS는 모든 시스템 메모리를 소유한다고 생각하지만 실제로는 각 메모리 액세스가 2 단계 변환에 의해 실제 메모리에 매핑됩니다. `vttbr_el2`는 2 단계 변환을위한 변환테이블의 기본 주소를 보유합니다. 이 시점에서 2 단계 변환이 비활성화되고`vttbr_el2`는 0으로 설정되어야합니다.

```
    cbz    x2, install_el2_stub

    mov    w0, #BOOT_CPU_MODE_EL2        // This CPU booted in EL2
    isb
    ret
```

첫 번째`x2`는`0`과 비교되어 VHE가 활성화되어 있는지 확인합니다. 그렇다면`install_el2_stub` 레이블로 이동하십시오. 그렇지 않으면 CPU가 EL2 모드로 부팅 된 것을 기록하고`el2_setup` 기능을 종료하십시오. 후자의 경우 프로세서는 계속 EL2 모드에서 작동하며 EL1은 전혀 사용되지 않습니다.

```
install_el2_stub:
    /* sctlr_el1 */
    mov    x0, #0x0800            // Set/clear RES{1,0} bits
CPU_BE(    movk    x0, #0x33d0, lsl #16    )    // Set EE and E0E on BE systems
CPU_LE(    movk    x0, #0x30d0, lsl #16    )    // Clear EE and E0E on LE systems
    msr    sctlr_el1, x0

```

이 시점에 도달하면 VHE가 필요하지 않고 곧 EL1로 전환 할 것이므로 미리 EL1 초기화를 수행해야합니다. 복사 된 코드 단편은`sctlr_el1` (시스템 제어 레지스터) 초기화를 담당합니다. 우리는 이미 RPi OS에서 [동일한 작업](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/boot.S#L18)을 수행했습니다.

```
    /* Coprocessor traps. */
    mov    x0, #0x33ff
    msr    cptr_el2, x0            // Disable copro. traps to EL2
```

이 코드를 통해 EL1은 `cpacr_el1` 레지스터에 액세스 할 수 있으며 결과적으로 Trace, Floating-point 및 Advanced SIMD 기능에 대한 접근을 제어 할 수 있습니다.

```
    /* Hypervisor stub */
    adr_l    x0, __hyp_stub_vectors
    msr    vbar_el2, x0
```

일부 기능에는 필요하지만 지금은 EL2를 사용할 계획이 없습니다. 하지만 우리는 현재 실행중인 커널에서 다른 커널로 로드하고 부팅 할 수있는 [kexec](https://linux.die.net/man/8/kexec) 시스템 호출을 구현하려면 EL2가 필요합니다.

[_hyp_stub_vectors](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/hyp-stub.S#L33)는 EL2에 대한 모든 예외 처리기(Exception handler)의 주소를 가지고 있습니다. 다음 단원에서는 인터럽트 및 예외 처리에 대해 자세히 설명한 후 EL1에 대한 예외 처리 기능을 구현할 것입니다.

```
    /* spsr */
    mov    x0, #(PSR_F_BIT | PSR_I_BIT | PSR_A_BIT | PSR_D_BIT |\
              PSR_MODE_EL1h)
    msr    spsr_el2, x0
    msr    elr_el2, lr
    mov    w0, #BOOT_CPU_MODE_EL2        // This CPU booted in EL2
    eret
```

마지막으로 EL1에서 프로세서 상태를 초기화하고 예외 수준을 전환해야합니다. 우리는 이미 [RPi OS](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/boot.S#L27-L33) 에서 이 작업을 했으므로이 코드의 세부 사항을 설명하지 않을 것입니다.

여기서 유일하게 새로운 것은`elr_el2`가 초기화되는 방법입니다. `lr` 또는 링크 레지스터는`x30`의 별칭입니다. `bl` (Branch Link) 명령어를 실행할 때마다`x30`은 현재 명령어의 주소로 자동으로 채워집니다. 사실 일반적으로 `ret` 명령에 의해 사용되므로 정확히 어디로 돌아갈 지 알고 있습니다. 우리의 경우 lr은 [여기](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L119)를 가리키며 `elr_el2`를 초기화하는 방식 때문에 EL1로 전환 한 후 실행이 재개되는 곳이기도합니다.

### Processor initialization at EL1 (EL1에서 프로세서 초기화)

이제 `stext` 함수로 돌아갑니다. 다음 몇 줄은 우리에게 별로 중요하지 않지만 온전한  설명을 위해 짚고 넘어 가겠습니다.

```
    adrp    x23, __PHYS_OFFSET
    and    x23, x23, MIN_KIMG_ALIGN - 1    // KASLR offset, defaults to 0
```
[KASLR](https://lwn.net/Articles/569635/)(Kernel Address Space Layout Randomization)은 커널을 메모리의 임의 주소에 배치 할 수있는 기술입니다. 이것은 오직 보안상의 이유로 필요합니다. 자세한 내용은 위의 링크를 읽으십시오.

```
    bl    set_cpu_boot_mode_flag
```

여기서 CPU 부팅 모드는  [__boot_cpu_mode](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/virt.h#L74) 변수에 저장됩니다. 이 작업을 수행하는 코드는 이전에 살펴본 `preserve_boot_args` 함수와 매우 유사합니다.

```
    bl    __create_page_tables
    bl    __cpu_setup            // initialise processor
    b    __primary_switch
```

마지막 3 가지 기능은 매우 중요하지만 모두 가상 메모리 관리와 관련되어 있으므로 6 장까지 세부적인 설명을 미루겠습니다. 지금은 단지 의미를 간략하게 설명하고자합니다.
* `__create_page_tables` 이름에서 알 수 있듯이 페이지 테이블을 생성합니다.
* `__cpu_setup` 가상 프로세서 관리에 특화된 다양한 프로세서 설정을 초기화합니다.
* `__primary_switch` MMU를 활성화하고 아키텍처 독립적 시작지점 인 start_kernel 함수로 이동합니다.

### 결론

이 장에서는 Linux 커널을 부팅 할 때 프로세서가 초기화되는 방법에 대해 간단히 설명했습니다. 다음 레슨에서도 계속 ARM 프로세서와 긴밀하게 작업하며 모든 OS의 핵심 주제 인 인터럽트 처리를 알아보겠습니다.
 
##### 이전 페이지

2.1 [Processor initialization: RPi OS](../lesson02/linux.md)

##### 다음 페이지

2.3 [Processor initialization: Exercises](../lesson03/rpi-os.md)

##### 추가 사항

이 문서는 훌륭한 오픈소스 운영체제 학습 프로젝트인 Sergey Matyukevich의 문서를 영어에 능숙하지 않은 한국인들이 학습할 수 있도록 번역한 것입니다. 오타 및 오역이 있을 수 있습니다. Sergey Matyukevich는 한국어를 능숙하게 다루지 못합니다. 대신 저에게 elxm6123@gmail.com으로 연락해주세요.
