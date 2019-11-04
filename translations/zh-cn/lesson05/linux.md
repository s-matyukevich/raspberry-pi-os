## 5.2: 用户流程和系统调用 

本章将是简短的一章。原因是我将`syscall`实现从Linux几乎精确地复制到了RPi OS，因此不需要太多解释。但是我仍然想引导您了解Linux源代码，以便您可以了解在何处以及如何实现特定的`syscall`功能。

### 创建第一个用户进程

我们要解决的第一个问题是如何创建第一个用户进程。一个开始寻找答案的好地方是 [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) 函数 - 正如我们先前所见, 这是第一个与体系结构无关的功能，在内核引导过程的早期就被调用。这是内核初始化开始的地方，在内核初始化期间运行第一个用户进程是有意义的。

确实，如果遵循`start_kernel`的逻辑，您很快就会发现 [kernel_init](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L989) 具有以下代码的函数。

```
    if (!try_to_run_init_process("/sbin/init") ||
        !try_to_run_init_process("/etc/init") ||
        !try_to_run_init_process("/bin/init") ||
        !try_to_run_init_process("/bin/sh"))
        return 0;
```

看起来这正是我们想要的。从该代码中，我们还可以推断Linux内核在何处以及以什么顺序查找 `init` 用户程序。 `try_to_run_init_process` 然后执行 [do_execve](https://github.com/torvalds/linux/blob/v4.14/fs/exec.c#L1837) 函数, 这也负责处理 [execve](http://man7.org/linux/man-pages/man2/execve.2.html) 系统调用. 该系统调用读取二进制可执行文件，并在当前进程中运行它。

在第9课中将详细探讨 `execve` 系统调用背后的逻辑，到目前为止，足以提及该 `syscall` 所做的最重要的工作是解析可执行文件并将其内容加载到内存中，并且在里面完成 [load_elf_binary](https://github.com/torvalds/linux/blob/v4.14/fs/binfmt_elf.c#L679) 功能. (在这里，我们假设可执行文件位于 [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) 格式 - 它是最受欢迎的, 但不是唯一的选择) 在 `load_elf_binary` 方法的最后(其实 [这里](https://github.com/torvalds/linux/blob/v4.14/fs/binfmt_elf.c#L1149)) 有特定于体系结构的 [start_thread](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L119) 函数. 我将其用作RPi OS`move_to_user_mode`例程的原型，这是我们最关心的代码。这里是。

```cpp
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

在执行 `start_thread` 时，当前进程将以内核模式运行。允许 `start_thread` 访问当前的 `pt_regs` 结构，该结构用于设置已保存的 `pstate`，`sp` 和 `pc` 字段。这里的逻辑与RPi OS的`move_to_user_mode`函数完全相同，所以我不想再重复一遍。要记住的重要一件事是，`start_thread`会以这样的方式准备已保存的处理器状态：`kernel_exit`宏最终会将进程移至用户模式。

###  Linux 系统调用

令您惊讶的是，主要的系统调用机制在 `Linux` 和 RPi OS中完全相同。 现在我们将使用已经熟悉的 [clone](http://man7.org/linux/man-pages/man2/clone.2.html) syscall 了解此机制的详细信息。 从我们的探索开始是有意义的 [glibc clone wrapper function](https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/unix/sysv/linux/aarch64/clone.S;h=e0653048259dd9a3d3eb3103ec2ae86acb43ef48;hb=HEAD#l35). 它的工作原理与 [call_sys_clone](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L22) 函数在RPi OS中，除了前一个函数执行者对完整性检查进行论证并正确处理异常外。要记住和理解的重要一点是，在两种情况下，我们都使用`svc`指令生成同步异常，使用`x8`寄存器传递`syscall` 编号，并且将所有参数传递到寄存器 `x0` - `x7` 中。

接下来，让我们看看如何定义`clone` syscall。 可以找到定义在 [这里](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2153) 看起来像这样

```cpp
SYSCALL_DEFINE5(clone, unsigned long, clone_flags, unsigned long, newsp,
         int __user *, parent_tidptr,
         int __user *, child_tidptr,
         unsigned long, tls)
{
    return _do_fork(clone_flags, newsp, 0, parent_tidptr, child_tidptr, tls);
}
```
[SYSCALL_DEFINE5](https://github.com/torvalds/linux/blob/v4.14/include/trace/syscall.h#L25) 宏名称中的数字为`5`，表示我们正在定义具有`5`个参数的`syscall`。  宏分配并填充新的 [syscall_metadata](https://github.com/torvalds/linux/blob/v4.14/include/trace/syscall.h#L25) 结构和创造 `sys_<syscall name>` 函数. 例如，在定义`clone` syscall的情况下，将定义`sys_clone`函数-这是将从底层架构代码中调用的实际 `syscall` 处理程序。

执行 `syscall` 时，内核需要一种通过 `syscall` 编号查找 `syscall` 处理程序的方法。实现此目的的最简单方法是创建一个指向 `syscall` 处理程序的指针数组，并使用 `syscall` 号作为该数组中的索引。这是我们在RPi OS中使用的方法，与Linux内核中使用的方法完全相同。 指向 `syscall` 处理程序的指针数组 [sys_call_table](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/sys.c#L62) 并定义如下。

```
void * const sys_call_table[__NR_syscalls] __aligned(4096) = {
    [0 ... __NR_syscalls - 1] = sys_ni_syscall,
#include <asm/unistd.h>
};
``` 

最初，所有系统调用都被分配为指向 `sys_ni_syscall` 函数（在此，`ni` 表示“不存在”）。编号为`0` 的 `Syscall` 以及尚未为当前体系结构定义的所有 `Syscall` 都将保留此处理程序。 `sys_call_table` 数组中的所有其他`syscall`处理程序都将在 [asm/unistd.h](https://github.com/torvalds/linux/blob/v4.14/include/uapi/asm-generic/unistd.h) 头文件。 如您所见，此文件仅提供 `syscall` 编号和 `syscall` 处理函数之间的映射。

### 低级系统调用处理代码

好的，我们已经了解了如何创建和填充`sys_call_table`，现在是时候研究底层 `syscall` 处理代码如何使用它了。而且这里的基本机制将与RPi OS中的几乎完全相同。

我们知道任何系统调用都是同步异常，并且所有异常处理程序都在异常向量表中定义 (这是我们的最爱 [vectors](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L367) 数组). 我们感兴趣的处理程序应该是处理在 `EL0` 处生成的同步异常的处理程序。 所有这些使找到合适的处理程序变得轻而易举，它被称为 [el0_sync](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L598) 如下所示。

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

这里的 `esr_el1`（异常征兆寄存器）用于确定当前异常是否是系统调用。 如果是这样的话 [el0_svc](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L837) 函数被调用。此功能在下面列出。

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

前3行会初始化变量`stbl`，`wscno`和`wsc_nr`，这些变量只是某些寄存器的别名。 `stbl` 保存系统调用表的地址，`wsc_nr` 包含系统调用的总数，`wscno` 是来自 `w8` 寄存器的当前系统调用号。

```
    stp    x0, xscno, [sp, #S_ORIG_X0]    // save the original x0 and syscall number
```

正如你可能从我们的RPI OS系统调用的讨论，`x0`是一个系统调用完成后覆盖在`pt_regs`区域记得。在时可能需要的`x0`寄存器的初始值的情况下，它被保存在`pt_regs`结构的独立领域。同样，系统调用号也保存在pt_regs中。

```
    enable_dbg_and_irq
```

启用了中断和调试异常。

```
    ct_user_exit 1
``` 

记录了从用户模式切换到内核模式的事件。

```
    ldr    x16, [tsk, #TSK_TI_FLAGS]    // check for syscall hooks
    tst    x16, #_TIF_SYSCALL_WORK
    b.ne    __sys_trace

```

如果当前任务在系统调用跟踪器下执行，则应设置 `_TIF_SYSCALL_WORK` 标志。在这种情况下，将调用 `__sys_trace` 函数。由于我们的讨论仅针对一般情况，因此我们将跳过此功能。

```
    cmp     wscno, wsc_nr            // check upper syscall limit
    b.hs    ni_sys
```

如果当前系统调用数大于系统调用总数，则会向用户返回错误。

```
    ldr    x16, [stbl, xscno, lsl #3]    // address in the syscall table
    blr    x16                // call sys_* routine
    b    ret_fast_syscall
```

`Syscall` 编号用作 `syscall` 表数组中的索引以查找处理程序。然后将处理程序地址加载到`x16` 寄存器中并执行。最后调用 `ret_fast_syscall`函数。

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

这里重要的是第一行，中断被禁用，最后一行，`kernel_exit`-其他所有与特殊情况处理有关。因此，您可能会猜到这是系统调用实际完成并将执行转移到用户进程的地方。

### 结论

现在，我们已经完成了生成和处理系统调用的过程。这个过程相对简单，但是对操作系统至关重要，因为它允许内核设置API，并确保该API是用户程序与内核之间通信的唯一手段。

##### Previous Page

5.1 [用户进程和系统调用：RPi OS](../../docs/lesson05/rpi-os.md)

##### Next Page

5.3 [用户流程和系统调用：练习](../../docs/lesson05/exercises.md)
