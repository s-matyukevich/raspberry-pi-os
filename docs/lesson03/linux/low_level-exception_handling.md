## 3.2: Low-level exception handling in Linux

Given huge Linux kernel source code, what is a good way to find the code that is responsible for interrupt handling? I can suggest one idea. Vector table base address should be stored in the 'vbar_el1' register, so, if you search for `vbar_el1`, you should be able to figure out where exactly the vector table is initialized. Indeed, the search gives us just a few usages, one of which belongs to already familiar to us [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S). This code is inside [__primary_switched](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L323) function. This function is executed after the MMU is switched on. The code looks like the following

```
    adr_l    x8, vectors            // load VBAR_EL1 with virtual
    msr    vbar_el1, x8            // vector table address
``` 

From this code, we can infer that the vector table is called `vectors` and you should be able to easily find [its definition](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L367).

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

Looks familiar, isn't it? And indeed, I've copied most of this code and just simplified it a little bit. [kernel_ventry](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L72) macro is almost the same as [ventry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L12), defined in the RPi OS. One difference, though, is that `kernel_ventry`  also is responsible for checking whether a kernel stack overflow has occurred. This functionality is enabled if `CONFIG_VMAP_STACK` is set and it is a part of the kernel feature that is called `Virtually mapped kernel stacks`. I'm not going to explain it in details here, however, if you are interested, I can recommend you to read [this](https://lwn.net/Articles/692208/) article.

### kernel_entry

[kernel_entry](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L120) macro should also be familiar to you. It is used exactly in the same way as the [corresonding macro](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L17) in the RPI OS. Original (Linux) version, however, is a lot more complicated. The code is listed below. 

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

Now we are going to explore the `kernel_entry` macro in details.

```
    .macro    kernel_entry, el, regsize = 64
``` 

The macro accepts 2 parameters: `el` and `regsize`. `el` can be either `0` or `1` depending on whether an exception was generated at EL0 or EL1. `regsize` is 32 if we came from 32-bit EL0 or 64 otherwise.

```
    .if    \regsize == 32
    mov    w0, w0                // zero upper 32 bits of x0
    .endif
```

In 32-bit mode, we use 32-bit general purpose registers (`w0` instead of `x0`).  `w0` is architecturally mapped to the lower part of `x0`. The provided code snippet zeroes upper 32 bits of the `x0` register by writing `w0` to itself

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

This part saves all general purpose registers on the stack. Note, that stack pointer was already adjusted in the [kernel_ventry](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L74) fo fit everything that needs to be stored. The order in which we save registers matters because in Linux there is a special structure [pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L119) that is used to access saved registers later inside an exception handler. As you might see this structure contains not only general purpose registers but also some other information, which is mostly populated later in the `kernel_entry` macro. I recommend you to remember `pt_regs` struct because we are going to implement and use a similar one in the next few lessons.

```
    .if    \el == 0
    mrs    x21, sp_el0
```

`x21` now contains aborted stack pointer. Note, that a task in Linux uses 2 different stacks for user and kernel mode. In case of user mode, we can use `sp_el0` register to figure out the stack pointer value at the moment when the exception was generated. This line is very important because we need to swap stack pointers during the context switch. We will talk about it in details in the next lesson.

```
    ldr_this_cpu    tsk, __entry_task, x20    // Ensure MDSCR_EL1.SS is clear,
    ldr    x19, [tsk, #TSK_TI_FLAGS]    // since we can unmask debug
    disable_step_tsk x19, x20        // exceptions when scheduling.
```

`MDSCR_EL1.SS` bit is responsible for enabling "Software Step exceptions". If this bit is set and debug exceptions are unmasked, an exception is generated after any instruction has been executed. This is commonly used by debuggers. When taking exception from user mode, we need to check first whether [TIF_SINGLESTEP](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L93) flag is set for the current task. If yes, this indicates that the task is executing under a debugger and we must unset `MDSCR_EL1.SS` bit.
The important thing to understand in this code is how information about the current task is obtained. In Linux, each process or thread (later I will reference any of them as just "task") has a [task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L519) associated with it. This struct contains all metadata information about a task On `arm64` architecture `task_struct` embeds another structure that is called [thread_info](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L39) so that a pointer to `task_struct` can always be used as a pointer to `thread_info`. `thread_info` is the place were flags are stored along with some other low-level values that `entry.S` need direct access to. 

```
    mov    x29, xzr            // fp pointed to user-space
```

Though `x29` is a general purpose register it usually has a special meaning. It is used as a "Frame pointer". Now I want to spend some time to explain its purpose. 

When a function is compiled, the first couple of instructions are usually responsible for storing old frame pointer and link register values on the stack. (Just a quick reminder: `x30` is called link register and it holds a "return address" that is used by the `ret` instruction) Then a new stack frame is allocated, so that it can contain all local variables of the function, and frame pointer register is set to point to the bottom of the frame. Whenever the function needs to access some local variable it simply adds hardcoded offset to the frame pointer. Imagine now that an error has occurred and we need to generate a stack trace. We can use current frame pointer to find all local variables in the stack, and the link register can be used used to figure out the precise location of the caller. Next, we take advantage of the fact that old frame pointer and link register values are always saved at the beginning of the stack frame, and we just read them from there. After we get caller's frame pointer we can now access all its local variables as wells. This process is repeated recursively until we reach the top of the stack and is called "stack unwinding". A similar algorithm is used by [ptrace](http://man7.org/linux/man-pages/man2/ptrace.2.html) system call.

Now, going back to the `kernel_entry` macro, it should be clear why do we need to clear `x29` register after taking an exception from EL0. That is because in Linux each task uses a different stack for user and kernel mode, and therefore it doesn't make sense to have common stack traces. 

```
    .else
    add    x21, sp, #S_FRAME_SIZE
```

Now we are inside else clause, which mean that this code is relevant only if we are handling an exception taken from EL1. In this case, we are reusing old stack and the provided code snippet just saves original `sp` value in the `x21` register for later usage.

```
    /* Save the task's original addr_limit and set USER_DS (TASK_SIZE_64) */
    ldr    x20, [tsk, #TSK_TI_ADDR_LIMIT]
    str    x20, [sp, #S_ORIG_ADDR_LIMIT]
    mov    x20, #TASK_SIZE_64
    str    x20, [tsk, #TSK_TI_ADDR_LIMIT]
```

Task address limit specifies the largest virtual address that can be used. When user process operates in 32-bit mode this limit is `2^32`. For 64 bit kernel it can be larger and usually is `2^48`. If it happens that an exception is taken from 32-bit EL1, task address limit need to be changed to [TASK_SIZE_64](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/memory.h#L80). Also, it is required to save the original address limit because it needs to be restored before the execution will be returned to user mode. 

```
    mrs    x22, elr_el1
    mrs    x23, spsr_el1
```

`elr_el1` and `spsr_el1` must be saved on the stack before we start handling an exception. We haven't done it yet in the RPI OS, because for now we always return to the same location from which an exception was taken. But what if we need to do a context switch while handling an exception? We will discuss this scenario in details in the next lesson.

```
    stp    lr, x21, [sp, #S_LR]
```

Link register and frame pointer registers are saved on the stack. We already saw that frame pointer is calculated differently depending on whether an exception was taken from EL0 or EL1 and the result of this calculation was already stored in `x21` register.

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

Here [stackframe](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L140) property of the `pt_regs` struct is filled. This property also contains link register and frame pointer, though this time the value of `elr_el1` (which is now in `x22`) is used instead of `lr`. `stackframe` is used solely for stack unwinding.

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

`CONFIG_ARM64_SW_TTBR0_PAN` parameter prevents the kernel from accessing user-space memory directly.  If you are wondering when this might be useful you can read [this](https://kernsec.org/wiki/index.php/Exploit_Methods/Userspace_data_usage) article. For now, I will also skip the detailed explanation of how this works, because such security features are too out of scope for our discussion.

```
    stp    x22, x23, [sp, #S_PC]
```

Here `elr_el1` and `spsr_el1` are saved on the stack.

```
    /* Not in a syscall by default (el0_svc overwrites for real syscall) */
    .if    \el == 0
    mov    w21, #NO_SYSCALL
    str    w21, [sp, #S_SYSCALLNO]
    .endif
```

`pt_regs` struct has a [field](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L132) indicating whether the current exception is a system call or not. By default, we assume that it isn't. Wait till lecture 5 for the detailed explanation how syscalls work.

```
    /*
     * Set sp_el0 to current thread_info.
     */
    .if    \el == 0
    msr    sp_el0, tsk
    .endif
```

When a task is executed in kernel mode, `sp_el0` is not needed. Its value was previously saved on the stack so it can be easily restored in `kernel_exit` macro. Starting from this point `sp_el0` will be used to hold a pointer to current [task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L519) for quick access.

### el1_irq

Next thing we are going to explore is the handler that is responsible for processing IRQs taken from EL1. From the [vector table](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L374) we can easily find out that the handler is called `el1_irq` and is defined [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L562). Let's take a look on the code now and examine it line by line.

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

The following is done inside this function.

* `kernel_enter` and `kernel_exit` macros are called to save and restore processor state. The first parameter indicates that the exception is taken from EL1.
* Debug interrupts are unmasked by calling `enable_dbg` macro. At this point, it is safe to do so, because the processor state is already saved and, even if debug exception occurred in the middle of the interrupt handler, it will be processed correctly. If you wonder why is it necessary to unmask debug exceptions during an interrupt processing in the first place - read [this](https://github.com/torvalds/linux/commit/2a2830703a2371b47f7b50b1d35cb15dc0e2b717) commit message. 
* Code inside `#ifdef CONFIG_TRACE_IRQFLAGS` block is responsible for tracing interrupts. It records 2 events: interrupt start and end. 
* Code inside `#ifdef CONFIG_PREEMPT` block access current task flags to check whether we need to call the scheduler. This code will be examined details in the next lesson. 
* `irq_handler` - this is the place were actual interrupt handling is performed. 

[irg_handler](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L351) is a macro and it is defined as the follows.

```
    .macro    irq_handler
    ldr_l    x1, handle_arch_irq
    mov    x0, sp
    irq_stack_entry
    blr    x1
    irq_stack_exit
    .endm
```

As you might see from the code, `irq_handler` executes [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44)  function. This function is executed with special stack, that is called "irq stack".  Why is it necessary to switch to a different stack? In RPI OS, for example, we didn't do this.  Well, I guess it is not necessary, but without it, an interrupt will be handled using task stack, and we can never be sure how much of it is still left for the interrupt handler.

Next, we need to look at [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44) It appears that it is not a function, but a variable. It is set inside [set_handle_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L46) function. But who sets it, and what is the fade of an interrupt after it reaches this point? We will figure out the answer in the next chapter of this lesson.  

### Conclusion

As a conclusion, I can say that we've already explored the low-level interrupt handling code and trace the path of an interrupt from the vector table all the way to the `handle_arch_irq`. This is the point were an interrupt leaves architecture specific code and started to be handled by a driver code. Our goal in the next chapter will be to trace the path of a timer interrupt through the driver source code. 

