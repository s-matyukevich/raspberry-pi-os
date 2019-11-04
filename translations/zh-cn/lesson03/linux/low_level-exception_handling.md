## 3.2: Linux中的低级异常处理

给定庞大的Linux内核源代码，找到负责中断处理的代码的好方法是什么？我可以提出一个想法。 向量表的基地址应存储在`vbar_el1` 寄存器中，因此，如果搜索 `vbar_el1`，则应该能够弄清楚向量表的初始化位置。 确实，搜索仅给我们提供了一些用法，其中之一属于我们已经熟悉的[head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/头) 这段代码位于[__primary_switched](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L323) 函数内部。 `MMU` 打开后执行此功能。该代码如下所示。

```
    adr_l    x8, vectors            // load VBAR_EL1 with virtual
    msr    vbar_el1, x8            // vector table address
```

从这段代码中，我们可以推断出向量表被称为 `向量`，您应该能够轻松找到[其定义](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L367)。

```cpp
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

看起来很熟悉，不是吗？实际上，我已经复制了大部分代码，并将其简化了一些。 [kernel_ventry]（https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L72）宏与[ventry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S＃L12)在RPi OS中定义。不过，一个区别是`kernel_ventry`还负责检查是否发生了内核堆栈溢出。如果设置了`CONFIG_VMAP_STACK`，则启用此功能，它是内核功能的一部分，称为虚拟映射内核堆栈。 我不会在这里详细解释它，但是，如果您有兴趣，我建议您阅读[这](https://lwn.net/Articles/692208/)文章。

### kernel_entry

[kernel_entry](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L120)宏对您也应该很熟悉。 与[相应宏](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L17) 的使用方式完全相同。 RPI操作系统。但是，原始（Linux）版本要复杂得多。 该代码在下面列出。

```cpp
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

现在，我们将详细研究`kernel_entry`宏。

```
    .macro    kernel_entry, el, regsize = 64
```

该宏接受2个参数：`el` 和 `regsize` 。 `el` 可以是 `0` 或 `1`，具体取决于是否在EL0或EL1上生成了异常。如果我们来自32位EL0，则 `regsize` 为32，否则为64。

```
    .if    \regsize == 32
    mov    w0, w0                // zero upper 32 bits of x0
    .endif
```

在32位模式下，我们使用32位通用寄存器（`w0`而不是`x0`）。 `w0`在结构上映射到`x0`的下部。所提供的代码片段通过向自身写入w0来将x0寄存器的高32位清零。

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

这部分将所有通用寄存器保存在堆栈中。请注意，在[kernel_ventry](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L74) 中已经对堆栈指针进行了调整，以适应所有需要被存储。保存寄存器的顺序很重要，因为在Linux中有一种特殊的结构[pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L1190)，用于稍后在异常处理程序中访问保存的寄存器。如您所见，该结构不仅包含通用寄存器，还包含其他一些信息，这些信息大多数稍后会在`kernel_entry`宏中填充。我建议您记住`pt_regs`结构，因为在接下来的几课中我们将实现并使用类似的结构。

```
    .if    \el == 0
    mrs    x21, sp_el0
```

x21现在包含异常终止的堆栈指针。请注意，Linux中的任务将两个不同的堆栈用于用户和内核模式。在用户模式下，我们可以使用 `sp_el0` 寄存器来计算产生异常时的堆栈指针值。这行非常重要，因为我们需要在上下文切换期间交换堆栈指针。在下一课中，我们将详细讨论。

```
    ldr_this_cpu    tsk, __entry_task, x20    // Ensure MDSCR_EL1.SS is clear,
    ldr    x19, [tsk, #TSK_TI_FLAGS]    // since we can unmask debug
    disable_step_tsk x19, x20        // exceptions when scheduling.
```

`MDSCR_EL1.SS`位负责启用 “软件步骤异常”。如果该位置1并且调试异常未屏蔽，则在执行任何指令后都会生成异常。 这是调试器通常使用的。 从用户模式获取异常时，我们需要首先检查[TIF_SINGLESTEP](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L93) 标志为当前任务设置。 如果是，则表明任务正在调试器下执行，我们必须将`MDSCR_EL1.SS`位置1。
在此代码中要了解的重要一点是如何获取有关当前任务的信息。在Linux中，每个进程或线程（稍后我都会将它们称为“任务”）都有一个[task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched .h＃L519) 与其关联。 该结构包含有关任务的所有元数据信息。在`arm64`体系结构上`task_struct`嵌入另一个称为[thread_info](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L39)，以便始终可以将指向`task_struct`的指针用作指向`thread_info`的指针。  `thread_info` 是标志与 `entry.S` 需要直接访问的其他一些低层值一起存储的地方。

```
    mov    x29, xzr            // fp pointed to user-space
```

尽管`x29`是通用寄存器，但通常具有特殊含义。它用作`框架指针`。现在，我想花一些时间来解释其目的。

编译函数时，第一对指令通常负责在堆栈中存储旧的帧指针和链接寄存器值。 （快速提醒一下：`x30`被称为链接寄存器，它包含一个由`ret`指令使用的“返回地址”）然后分配一个新的堆栈帧，以便它可以包含函数的所有局部变量，并且帧指针寄存器设置为指向帧的底部。每当函数需要访问某些局部变量时，它仅向帧指针添加硬编码的偏移量。现在想象一下发生了一个错误，我们需要生成一个堆栈跟踪。我们可以使用当前帧指针在堆栈中查找所有局部变量，并且可以使用链接寄存器来确定调用者的确切位置。接下来，我们利用以下事实：旧的帧指针和链接寄存器的值始终保存在堆栈帧的开头，而我们只是从那里读取它们。获取调用者的帧指针之后，我们现在也可以访问其所有局部变量。递归地重复此过程，直到到达堆栈顶部为止，这称为“堆栈展开”。 [ptrace](http://man7.org/linux/man-pages/man2/ptrace.2.html) 系统调用使用了类似的算法。

现在，回到 `kernel_entry` 宏，应该很清楚为什么在从EL0中获取异常后为什么需要清除`x29`寄存器。这是因为在Linux中，每个任务在用户和内核模式下都使用不同的堆栈，因此拥有通用堆栈跟踪没有任何意义。

```
    .else
    add    x21, sp, #S_FRAME_SIZE
```

现在我们在else子句中，这意味着仅当我们处理从EL1提取的异常时，此代码才有意义。在这种情况下，我们将重用旧堆栈，所提供的代码段仅将原始`sp`值保存在`x21`寄存器中，以备后用。

```
    /* Save the task's original addr_limit and set USER_DS (TASK_SIZE_64) */
    ldr    x20, [tsk, #TSK_TI_ADDR_LIMIT]
    str    x20, [sp, #S_ORIG_ADDR_LIMIT]
    mov    x20, #TASK_SIZE_64
    str    x20, [tsk, #TSK_TI_ADDR_LIMIT]
```

任务地址限制指定了可以使用的最大虚拟地址。当用户进程以32位模式运行时，此限制为 `2^32`。对于64位内核，它可以更大，通常为`2^48`。如果碰巧从32位EL1中获取了异常，则需要将任务地址限制更改为[TASK_SIZE_64](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/memory.h＃L80)。此外，还需要保存原始地址限制，因为需要先还原该地址限制，然后才能将执行返回到用户模式。

```
    mrs    x22, elr_el1
    mrs    x23, spsr_el1
```

在开始处理异常之前，必须先将`elr_el1`和`spsr_el1`保存在堆栈中。我们尚未在RPI OS中完成此操作，因为到目前为止，我们始终返回到发生异常的位置。但是，如果我们需要在处理异常时进行上下文切换，该怎么办？在下一课中，我们将详细讨论这种情况。

```
    stp    lr, x21, [sp, #S_LR]
```

链接寄存器和帧指针寄存器保存在堆栈中。我们已经看到，根据是从`EL0`还是从`EL1`提取异常，帧指针的计算方式有所不同，并且该计算的结果已存储在x21寄存器中。

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

此处填充了`pt_regs`结构的[stackframe](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L140) 属性。该属性还包含链接寄存器和帧指针，尽管这次使用的是 `elr_el1` 的值（现在位于 `x22` 中）而不是 `lr`。 `stackframe`仅用于堆栈退卷。

```cpp
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

`CONFIG_ARM64_SW_TTBR0_PAN` 参数防止内核直接访问用户空间内存。如果您想知道什么时候有用，可以阅读[this](https://kernsec.org/wiki/index.php/Exploit_Methods/Userspace_data_usage)文章。现在，我还将跳过对此工作原理的详细说明，因为此类安全功能对于我们的讨论而言已超出范围。

```
    stp    x22, x23, [sp, #S_PC]
```

这里，`elr_el1`和`spsr_el1`被保存在堆栈中。

```
    /* Not in a syscall by default (el0_svc overwrites for real syscall) */
    .if    \el == 0
    mov    w21, #NO_SYSCALL
    str    w21, [sp, #S_SYSCALLNO]
    .endif
```

`pt_regs`结构体具有[field](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L132)，指示当前异常是否为系统打电话或不打电话。默认情况下，我们假设不是。等到第5课，再详细了解`syscalls`的工作原理。

```
    /*
     * Set sp_el0 to current thread_info.
     */
    .if    \el == 0
    msr    sp_el0, tsk
    .endif
```

在内核模式下执行任务时，不需要`sp_el0`。其值先前已保存在堆栈中，因此可以在`kernel_exit`宏中轻松恢复。从这一点开始，`sp_el0`将用于持有指向当前[task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L519)的指针访问。

### el1_irq

接下来要探讨的是负责处理从EL1提取的IRQ的处理程序。 从[向量表](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L374)中，我们可以轻松地发现该处理程序称为 `el1_irq` 并在[此处](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L562) 中定义。 现在让我们看一下代码，逐行检查它。

```cpp
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

在此函数内部执行以下操作。

* 调用`kernel_entry`和`kernel_exit`宏来保存和恢复处理器状态。第一个参数指示异常来自`EL1`。
* 调用`enable_dbg`宏可以屏蔽调试中断。此时，这样做是安全的，因为已经保存了处理器状态，即使在中断处理程序的中间发生了调试异常，也可以正确处理它。如果您想知道为什么首先需要在中断处理期间取消屏蔽调试异常，请阅读[这里](https://github.com/torvalds/linux/commit/2a2830703a2371b47f7b50b1d35cb15dc0e2b717)提交消息。
* `#ifdef CONFIG_TRACE_IRQFLAGS`块中的代码负责跟踪中断。它记录2个事件：中断开始和结束。 
* `#ifdef CONFIG_PREEMPT`中的代码阻止访问当前任务标志，以检查我们是否需要调用调度程序。下一课将详细检查此代码。
* `irq_handler` - 这是执行实际中断处理的地方。

[irq_handler](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L351) 是一个宏，定义如下。

```
    .macro    irq_handler
    ldr_l    x1, handle_arch_irq
    mov    x0, sp
    irq_stack_entry
    blr    x1
    irq_stack_exit
    .endm
```

从代码中您可能会看到，`irq_handler`执行[handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44)函数。 该函数通过特殊的堆栈（称为 `irq堆栈`）执行。为什么有必要切换到其他堆栈？例如，在RPI OS中，我们没有这样做。好吧，我想这是没有必要的，但是如果没有它，将使用任务堆栈来处理中断，而且我们永远无法确定还有多少要留给中断处理程序。

接下来，我们需要查看 [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44)。看来这不是一个函数，而是一个变量。它在[set_handle_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L46) 函数中设置。但是谁来设置它，那么中断到达这一点后会逐渐消失吗？我们将在本课程的下一章中找到答案。

### 结论

总而言之，我可以说我们已经研究了低级中断处理代码，并从向量表一直跟踪到 `handle_arch_irq` 的中断路径。这就是中断离开体系结构特定代码并开始由驱动程序代码处理的关键所在。下一章的目标是通过驱动程序源代码跟踪计时器中断的路径。

##### 上一页

3.1 [中断处理：RPi OS](../../../docs/lesson03/rpi-os.md)

##### 下一页

3.3 [中断处理：中断控制器](../../../docs/lesson03/linux/interrupt_controllers.md)
