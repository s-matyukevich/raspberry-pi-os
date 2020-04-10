## 2.2: 处理器初始化 (Linux)

我们通过[stext](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L116) 函数停止了对 `Linux`  内核的探索.  arm64体系结构的概念. 这次, 我们将更深入一点, 并发现与在本课程和上一课程中已经实现的代码有一些相似之处. 

您可能会发现本章有些无聊, 因为它主要讨论了不同的`ARM`系统寄存器及其在`Linux`内核中的用法. 但是我仍然认为它非常重要, 原因如下：

1. 有必要了解硬件提供给软件的接口.  只需了解此接口, 您就可以在许多情况下解构如何实现特定的内核功能以及软件和硬件如何协作以实现此功能. 
2. 系统寄存器中的不同选项通常与启用/禁用各种硬件功能有关.  如果您了解不同的系统注册的`ARM`处理器, 那么您将已经知道它支持哪种功能. 

好的, 现在让我们继续对`stext`函数的研究. 

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

### preserve_boot_args

[preserve_boot_args](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L136) 函数负责保存由引导加载程序传递给内核的参数. 

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

据此[kernel boot protocol](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt#L150), 参数在寄存器`x0-x3`中传递给内核. `x0`包含系统RAM中设备树Blob(`.dtb`)的物理地址. `x1 - x3` 保留供将来使用. 该函数正在做的是将寄存器 `x0-x3` 的内容复制到 [boot_args](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/setup.c#L93) 数组接着使[无效](https://developer.arm.com/docs/den0024/latest/caches/cache-maintenance) 数据缓存到相应缓存行. 多处理器系统中的高速缓存维护本身就是一个大话题, 我们现在将略过它. 对于那些对此主题感兴趣的人, 我可以推荐阅读 [Caches](https://developer.arm.com/docs/den0024/latest/caches) 和 [Multi-core processors](https://developer.arm.com/docs/den0024/latest/multi-core-processors) 的`ARM Programmer’s Guide`章节 .

### el2_setup

据此 [arm64boot protocol](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt#L159), 内核可以在`EL1`或`EL2`中引导. 在第二种情况下, 内核可以访问虚拟化扩展, 并且可以充当主机操作系统.  如果我们有幸可以在EL2中启动, [el2_setup](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L386) 函数被调用. 它负责配置不同的参数(只能在EL2上访问), 并放到EL1上.  现在, 我将把这个功能分成几个小部分, 并逐一解释. 

```
    msr    SPsel, #1            // We want to use SP_EL{1,2}
``` 

专用堆栈指针将同时用于`EL1`和`EL2`. 另一个选择是重用`EL0`的堆栈指针. 

```
    mrs    x0, CurrentEL
    cmp    x0, #CurrentEL_EL2
    b.eq    1f
```

仅当当前EL为标签 `1` 的 `EL2` 分支时, 否则我们无法进行 `EL2` 设置, 并且此功能尚需完成. 

```
    mrs    x0, sctlr_el1
CPU_BE(    orr    x0, x0, #(3 << 24)    )    // Set the EE and E0E bits for EL1
CPU_LE(    bic    x0, x0, #(3 << 24)    )    // Clear the EE and E0E bits for EL1
    msr    sctlr_el1, x0
    mov    w0, #BOOT_CPU_MODE_EL1        // This cpu booted in EL1
    isb
    ret
```

如果发生这种情况, 我们将在`EL1`执行, `sctlr_el1` 寄存器已更新, 以便CPU可以根据[CPU_BIG_ENDIAN](https://github.com/torvalds/linux/blob/v4.14/arch/arm64 /Kconfig＃L612)的值在`little-endian`模式的`big-endian`模式下工作配置设置. 然后我们就退出`el2_setup`函数并返回[BOOT_CPU_MODE_EL1](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/virt.h#L55) 不变. 据此 [ARM64函数调用约定](http://infocenter.arm.com/help/topic/com.arm.doc.ihi0055b/IHI0055B_aapcs64.pdf) 返回值应放在`x0`寄存器中(在本例中为`w0`. 您可以将`w0`寄存器视为`x0`的前32位. ).

```
1:    mrs    x0, sctlr_el2
CPU_BE(    orr    x0, x0, #(1 << 25)    )    // Set the EE bit for EL2
CPU_LE(    bic    x0, x0, #(1 << 25)    )    // Clear the EE bit for EL2
    msr    sctlr_el2, x0
```

如果看来我们是在`EL2`中启动的, 则说明我们正在为`EL2`做相同的设置(请注意, 这次使用的是`sctlr_el2`寄存器, 而不是`sctlr_el1`. ). 

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

如果 [虚拟主机扩展 (VHE)](https://developer.arm.com/products/architecture/a-profile/docs/100942/latest/aarch64-virtualization) 已启用通过 [ARM64_VHE](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/Kconfig#L926) config变量, 并且主机支持它们, 然后将`x2`更新为非零值. `x2`将用于检查以后是否在同一功能中启用了`VHE`. 

```
    mov    x0, #HCR_RW            // 64-bit EL1
    cbz    x2, set_hcr
    orr    x0, x0, #HCR_TGE        // Enable Host Extensions
    orr    x0, x0, #HCR_E2H
set_hcr:
    msr    hcr_el2, x0
    isb
```

这里我们设置 `hcr_el2` 寄存器. 我们使用相同的寄存器为RPi OS中的EL1设置64位执行模式. 这正是在提供的代码示例的第一行中所做的. 同样, 如果 x2！= 0, 这意味着VHE可用并且内核被配置为使用它, 那么`hcr_el2`也被用来启用`VHE`. 

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

下一段代码在上面的注释中得到了很好的解释. 我没有更多的补充. 

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

仅当`GICv3`可用并启用时, 才执行下一个代码段.  `GIC`代表通用中断控制器.  `GIC`规范的`v3`版本增加了一些功能, 这些功能在虚拟化环境中特别有用. 例如, 使用`GICv3`, 就有可能具有`LPI`(本地特定的外围设备中断). 此类中断通过消息总线进行路由, 其配置保存在内存中的特殊表中. 

提供的代码负责启用SRE(系统寄存器接口). 必须执行此步骤, 然后我们才能使用`ICC _ *_ ELn`寄存器并利用GICv3功能. 

```
    /* Populate ID registers. */
    mrs    x0, midr_el1
    mrs    x1, mpidr_el1
    msr    vpidr_el2, x0
    msr    vmpidr_el2, x1
```

`midr_el1`和 `mpidr_el1` 是标识寄存器组中的只读寄存器. 它们提供了有关处理器制造商, 处理器体系结构名称, 内核数量以及其他一些信息的各种信息. 可以为所有尝试从`EL1`访问它的读者更改此信息. 在这里, 我们使用从`midr_el1`和`mpidr_el1`获取的值填充`vpidr_el2`和 `vmpidr_el2`, 因此无论您尝试从`EL1`还是更高级别的异常级别访问它, 此信息都是相同的. 

```
#ifdef CONFIG_COMPAT
    msr    hstr_el2, xzr            // Disable CP15 traps to EL2
#endif
```

当处理器以32位执行模式执行时, 存在 `协处理器` 的概念. 协处理器可用于访问信息, 该信息通常在64位执行模式下通过系统寄存器访问. 您可以在协处理器中[在官方文档中](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0311d/I1014521.html)了解确切可访问的内容.  `msrhstr_el2, xzr` 指令允许从较低的异常级别使用协处理器. 仅当启用兼容模式时才有意义(在这种模式下, 内核可以在64位内核之上运行32位用户应用程序. ). 

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

这段代码负责配置 `mdcr_el2`(监视器调试配置寄存器(EL2)). 该寄存器负责设置与虚拟化扩展相关的不同调试陷阱.  我将不解释此代码块的详细信息, 因为调试和跟踪在我们的讨论范围之外.  如果您对细节感兴趣, 我建议您阅读第2810页的 `mdcr_el2` 寄存器的描述.  [AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/ca/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile).

```
    /* Stage-2 translation */
    msr    vttbr_el2, xzr
```

当您的操作系统用作虚拟机管理程序时, 应为其来宾操作系统提供完全的内存隔离.  阶段2虚拟内存转换正是用于此目的：每个来宾OS都认为它拥有所有系统内存, 尽管实际上每个内存访问都是通过阶段2转换映射到物理内存的.  `vttbr_el2`存放第2阶段翻译的翻译表的基地址.   此时, 第2阶段转换被禁用, 并且`vttbr_el2`应该设置为0. 

```
    cbz    x2, install_el2_stub

    mov    w0, #BOOT_CPU_MODE_EL2        // This CPU booted in EL2
    isb
    ret
```

首先将`x2`与`0`进行比较, 以检查是否启用了`VHE`. 如果是, 则跳转至 `install_el2_stub` 标签, 否则记录 `CPU` 以 `EL2` 模式启动并退出 `el2_setup` 功能. 在后一种情况下, 处理器将继续以EL2模式运行, 并且将完全不使用EL1. 

```
install_el2_stub:
    /* sctlr_el1 */
    mov    x0, #0x0800            // Set/clear RES{1,0} bits
CPU_BE(    movk    x0, #0x33d0, lsl #16    )    // Set EE and E0E on BE systems
CPU_LE(    movk    x0, #0x30d0, lsl #16    )    // Clear EE and E0E on LE systems
    msr    sctlr_el1, x0

```

如果达到这一点, 则意味着我们不需要VHE, 并且将很快切换到EL1, 因此需要在此处进行早期的EL1初始化.   复制的代码段负责“ sctlr_el1”(系统控制寄存器)的初始化.  我们已经做了同样适用于RPi OS的工作在[这](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/boot.S#L18) . 

```
    /* Coprocessor traps. */
    mov    x0, #0x33ff
    msr    cptr_el2, x0            // Disable copro. traps to EL2
```

该代码允许EL1访问`cpacr_el1`寄存器, 从而控制对跟踪, 浮点和高级SIMD功能的访问. 

```
    /* Hypervisor stub */
    adr_l    x0, __hyp_stub_vectors
    msr    vbar_el2, x0
```

尽管某些功能需要它, 但我们现在不打算使用EL2. 例如, 我们需要它来实现[kexec](https://linux.die.net/man/8/kexec)系统调用, 该调用使您能够从当前运行的内核加载并引导到另一个内核. 

[_hyp_stub_vectors](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/hyp-stub.S#L33)包含EL2所有异常处理程序的地址. 在我们详细讨论中断和异常处理之后, 我们将在下一课中为EL1实现异常处理功能. 

```
    /* spsr */
    mov    x0, #(PSR_F_BIT | PSR_I_BIT | PSR_A_BIT | PSR_D_BIT |\
              PSR_MODE_EL1h)
    msr    spsr_el2, x0
    msr    elr_el2, lr
    mov    w0, #BOOT_CPU_MODE_EL2        // This CPU booted in EL2
    eret
```

最后, 我们需要在EL1处初始化处理器状态并切换异常级别.  我们已经为 [RPi OS](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/boot.S#L27-L33) 因此, 我将不解释此代码的详细信息. 

唯一的新东西是如何初始化`elr_el2`. `lr` 或链接寄存器是 `x30` 的别名.  每当执行 `bl`(分支链接)指令时, `x30` 都会自动填充当前指令的地址.  该事实通常由`ret`指令使用, 因此它知道确切返回的位置.  就我们而言, `lr` 在 [这](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L119) 和 由于我们初始化`elr_el2`的方式, 这也是切换到EL1后将要恢复执行的地方. 

### EL1处的处理器初始化

现在我们回到`stext`函数. 接下来的几行对我们来说不是很重要, 但是为了完整起见, 我想解释一下. 

```
    adrp    x23, __PHYS_OFFSET
    and    x23, x23, MIN_KIMG_ALIGN - 1    // KASLR offset, defaults to 0
```

[KASLR](https://lwn.net/Articles/569635/), 或者也称 内核地址空间布局随机化, 是一种允许将内核放置在内存中随机地址处的技术.  仅出于安全原因才需要这样做. 有关更多信息, 您可以阅读上面的链接. 

```
    bl    set_cpu_boot_mode_flag
```

此处将CPU引导模式保存到 [__boot_cpu_mode](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/virt.h#L74) 变量. 执行此操作的代码与我们之前探讨的`preserve_boot_args`函数非常相似. 

```
    bl    __create_page_tables
    bl    __cpu_setup            // initialise processor
    b    __primary_switch
```

最后3个功能非常重要, 但是它们都与虚拟内存管理有关, 因此我们将把它们的详细探索推迟到第6课. 现在, 我只想简短地描述一下其中的含义. 
* `__create_page_tables` 顾名思义, 它负责创建页表. 
* `__cpu_setup` 初始化各种处理器设置, 主要针对虚拟内存管理. 
* `__primary_switch` 启用MMU并跳至 [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) 函数, 这是与体系结构无关的起点. 

### 结论

在本章中, 我们简要讨论了引导Linux内核时如何初始化处理器.  在下一课中, 我们将继续与ARM处理器紧密合作, 并研究任何OS的重要主题：中断处理. 
 
##### 上一页

2.1 [处理器初始化：RPi OS](../../docs/lesson02/rpi-os.md)

##### 下一页

2.3 [处理器初始化：练习](../../docs/lesson02/exercises.md)
