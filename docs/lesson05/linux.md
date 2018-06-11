## 5.2: User processes and system calls 

This chapter is going to be a short one. The reason is that I copied syscall implementation from Linux to RPi OS almost precisely, therefore it doesn't require a lot of explanations. But still I want to guide you through the Linux source code so that you can see where and how a particular syscall functionality is implemented.

### Creating first user process

The first question we are going to tackle is how the first user process is created.  A good place to start looking for the answer is [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) function - as we've seen previously, this is the first architecture independent function that is called early in the kernel boot process. This is where the kernel initialization begins, and it would make sense to run the first user process during kernel initialization.

An indeed, if you follow the logic of `start_kernel` you will soon discover [kernel_init](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L989) function that has the following code.

```
    if (!try_to_run_init_process("/sbin/init") ||
        !try_to_run_init_process("/etc/init") ||
        !try_to_run_init_process("/bin/init") ||
        !try_to_run_init_process("/bin/sh"))
        return 0;
```

It looks like this is precisely what we are looking for. From this code we can also infer where exactly and in which order Linux kernel looks for the `init` user program. `try_to_run_init_process` then executes [do_execve](https://github.com/torvalds/linux/blob/v4.14/fs/exec.c#L1837) function, which is also responsible for handling [execve](http://man7.org/linux/man-pages/man2/execve.2.html) system call. This system call reads a binary executable file and runs it inside the current process.

The logic behind the `execve` system call will be explored in details in the lesson 9, for now, it will be enough to mention that the most important work that this syscall does is parsing executable file and loading its content into memory, and it is done inside [load_elf_binary](https://github.com/torvalds/linux/blob/v4.14/fs/binfmt_elf.c#L679) function. (Here we assume that the executable file is in [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) format - it is the most popular, but not the only choice) At the end of `load_elf_binary` method (actually [here](https://github.com/torvalds/linux/blob/v4.14/fs/binfmt_elf.c#L1149)) there is a call to architecture specific [start_thread](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L119) function. I used it as a prototype for the RPi OS `move_to_user_mode` routine, and this is the code that we mostly care about. Here it is.

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

By the time when `start_thread` is executed, the current process operates in the kernel mode. `start_thred` is given access to the current `pt_regs` struct, which is used to set saved `pstate`, `sp` and `pc` fields. The logic here is exactly the same as in the RPi OS `move_to_user_mode` function, so I don't want to repeat it one more time. An important thing to remember is that `start_thread` prepares saved processor state in such a way, that `kernel_exit` macro will eventually move the process to user mode.

###  Linux syscalls

It would be no surprise to you that the primary syscall mechanism is exactly the same in Linux and RPi OS. Now we are going to use already familiar [clone](http://man7.org/linux/man-pages/man2/clone.2.html) syscall to understand the details of this mechanism. It would make sense to start our exploration with the [glibc clone wrapper function](https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/unix/sysv/linux/aarch64/clone.S;h=e0653048259dd9a3d3eb3103ec2ae86acb43ef48;hb=HEAD#l35). It works exactly the same as [call_sys_clone](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L22) function in the RPi OS, with the exception that the former function performers arguments sanity check and handles exceptions properly. An important thing to remember and understand is that in both cases we are using `svc` instruction to generate a synchronous exception, syscall number is passed using `x8` register and all arguments are passed in registers `x0` - `x7`.

Next, let's take a look at how `clone` syscall is defined. The definition can be found [here](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2153) and looks like this.

```
SYSCALL_DEFINE5(clone, unsigned long, clone_flags, unsigned long, newsp,
         int __user *, parent_tidptr,
         int __user *, child_tidptr,
         unsigned long, tls)
{
    return _do_fork(clone_flags, newsp, 0, parent_tidptr, child_tidptr, tls);
}
```
[SYSCALL_DEFINE5](https://github.com/torvalds/linux/blob/v4.14/include/trace/syscall.h#L25) macro has number 5 in its name indicating that we are defining a syscall with 5 parameters.  The macro allocates and populates new [syscall_metadata](https://github.com/torvalds/linux/blob/v4.14/include/trace/syscall.h#L25) struct and creates `sys_<syscall name>` function. For example, in case of the `clone`  syscall `sys_clone` function will be defined - this is the actuall syscall handler that will be called from the low-level architecture code.

When a syscall is executed, the kernel needs a way to find syscall handler by the syscall number. The easiest way to achieve this is to create an array of pointers to syscall handlers and use syscall number as an index in this array. This is the approach we used in the RPi OS and exactly the same approach is used in the Linux kernel. The array of pointers to syscall handlers is called [sys_call_table](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/sys.c#L62) and is defined like this.

```
void * const sys_call_table[__NR_syscalls] __aligned(4096) = {
    [0 ... __NR_syscalls - 1] = sys_ni_syscall,
#include <asm/unistd.h>
};
``` 

All syscalls are initially allocated to point to `sys_ni_syscall` function ("ni" here means "non-existent"). Syscall with number `0` and all syscalls that aren't defined for the current architecture will keep this handler. All other syscall handlers in the `sys_call_table` array are rewritten in the [asm/unistd.h](https://github.com/torvalds/linux/blob/v4.14/include/uapi/asm-generic/unistd.h) header file. As you might see, this file simply provide a mapping between syscall number and syscall handler function.

### Low-level syscall handling code

Ok, we've seen how `sys_call_table` is created and populated, now it is time to investigate how it is used by the low-level syscall handling code. And once again the basic mechanism here will be almost exactly the same as in the RPi OS. 

We know that any syscall is a synchronous exception and all exception handlers are defined in the exception vector table (it is our favorite [vectors](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L367) array) The handler that we are interested in should be the one that handles synchronous exceptions generated at EL0. All of this makes it trivial to find the right handler, it is called [el0_sync](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L598) and looks like the following.

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

Here `esr_el1` (exception syndrome register) is used to figure out whether the current exception is a system call. If this is the case [el0_svc](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L837) function is called. This function is listed below.

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

The first 3 lines initialize `stbl`, `wscno` and `wsc_nr` variables that are just aliases for some registers. `stbl` holds the address of the syscall table, `wsc_nr` contains the total number of system calls and `wscno` is the current syscall number from `w8` register. 

```
    stp    x0, xscno, [sp, #S_ORIG_X0]    // save the original x0 and syscall number
```

As you might remember from our RPi OS syscall discussion, `x0` is overwritten in the `pt_regs` area after a syscall finishes. In case when original value of the `x0` register might be needed, it is saved in the separate field of the `pt_regs` struct. Similarly syscall number is also saved in the `pt_regs`.

```
    enable_dbg_and_irq
```

Interrupts and debug exceptions are enabled.

```
    ct_user_exit 1
``` 

The event of switching from user mode to kernel mode is recorded.

```
    ldr    x16, [tsk, #TSK_TI_FLAGS]    // check for syscall hooks
    tst    x16, #_TIF_SYSCALL_WORK
    b.ne    __sys_trace

```

In case if the current task is executed under a syscall tracer `_TIF_SYSCALL_WORK` flag should be set. In this case, `__sys_trace` function will be called. As our discussion is only focused on the general case, we are going to skip this function.

```
    cmp     wscno, wsc_nr            // check upper syscall limit
    b.hs    ni_sys
```

If current syscall number is greater then total syscall count an error is returned to the user.

```
    ldr    x16, [stbl, xscno, lsl #3]    // address in the syscall table
    blr    x16                // call sys_* routine
    b    ret_fast_syscall
```

Syscall number is used as an index in the syscall table array to find the handler. Then handler address is loaded into `x16` register and it is executed. Finally `ret_fast_syscall` function is called. 

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
The important things here are the first line, were interrupts are disabled, and the last line, were `kernel_exit` is called - everything else is related to special case processing. So as you might guess this is the place where a system call actually finishes and execution is transfered to user process.

### Conclusion

We've now gone through the process of generating and processing a system call. This process is relatively simple, but it is vital for the OS, because it allows the kernel to set up an API and make sure that this API is the only mean of communication between a user program and the kernel.

##### Previous Page

5.1 [User processes and system calls: RPi OS](../../docs/lesson05/rpi-os.md)

##### Next Page

5.3 [User processes and system calls: Exercises](../../docs/lesson05/exercises.md)
