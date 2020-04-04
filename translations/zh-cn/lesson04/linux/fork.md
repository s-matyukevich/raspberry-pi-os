## 4.3: Forking a task

调度就是从可用任务列表中选择合适的任务来运行. 但是在调度程序能够完成其工作之前, 我们需要以某种方式填写此列表. 创建新任务的方式是本章的主题. 

现在, 我们只希望专注于内核线程, 并将对用户模式功能的讨论推迟到下一堂课. 但是, 并非在所有地方都有可能, 因此也准备学习一些有关在用户模式下执行任务的知识. 

### 初始化任务

启动内核后, 将运行一个任务：init任务. 相应的`task_struct`被定义在 [这里](https://github.com/torvalds/linux/blob/v4.14/init/init_task.c#L20) and is initialized by [INIT_TASK](https://github.com/torvalds/linux/blob/v4.14/include/linux/init_task.h#L226) 宏. 此任务对系统至关重要, 因为系统中的所有其他任务均源于该任务. 

### 创建新任务

在Linux中, 不可能从头开始创建新任务-而是从当前正在运行的任务派生所有任务. 现在, 正如我们从初始任务的来源所看到的那样, 我们可以尝试探索如何从中创建新任务. 

有四种创建新任务的方式. 

1. [fork](http://man7.org/linux/man-pages/man2/fork.2.html) 系统调用将创建当前进程的完整副本, 包括其虚拟内存, 并用于创建新进程(而非线程). 该系统调用已定义在 [这里](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2116).
1. [vfork](http://man7.org/linux/man-pages/man2/vfork.2.html) 系统调用与 `fork` 类似, 但是区别在于子代重用了父虚拟内存以及堆栈, 并且父代被阻塞, 直到子代执行完毕.  可以找到此系统调用的定义在 [这里](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2128).
1. [clone](http://man7.org/linux/man-pages/man2/clone.2.html) 系统调用是最灵活的系统调用-它也复制当前任务, 但它允许使用`flags` 参数自定义过程, 并允许配置子任务的入口点. 在下一课中, 我们将了解如何实现 `glibc` 克隆包装器功能-该包装器允许使用 `clone` 系统调用来创建新线程. 
1. 最后, [kernel_thread](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2109) 函数可用于创建新的内核线程. 

以上所有函数调用 [_do_fork](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2020), 接受以下参数. 

* `clone_flags` 标志用于配置派生行为. 可以找到标志的完整列表在 [这里](https://github.com/torvalds/linux/blob/v4.14/include/uapi/linux/sched.h#L8).
* `stack_start` 在`clone` `syscall`的情况下, 此参数指示新任务的用户堆栈的位置. 如果 `kernel_thread` 调用 `_do_fork` , 则此参数指向需要在内核线程中执行的函数. 
* `stack_size` 在 `arm64` 体系结构中, 仅当 `_do_fork` 由 `kernel_thread` 调用时, 才使用此参数. 它是指向需要传递给内核线程函数的参数的指针.  (是的, 我也发现最后两个参数的命名具有误导性)
* `parent_tidptr` `child_tidptr` 那两个参数仅在`clone` `syscall`中使用.  Fork会将子线程ID存储在父代内存中的 `parent_tidptr` 位置, 也可以将父代ID存储在 `child_tidptr` 位置. 
* `tls`  [Thread Local Storage 线程本地存储](https://en.wikipedia.org/wiki/Thread-local_storage)

### Fork procedure

接下来, 我要突出显示在 `_do_fork` 执行期间发生的最重要事件, 并保持其顺序. 

1. [_do_fork](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2020) 调用 [copy_process](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1539)  `copy_process`负责配置新的`task_struct`. 
1. `copy_process` 调用 [dup_task_struct](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L512), 分配新的`task_struct`并复制原始字段的所有字段.  实际复制发生在特定于体系结构的地方 [arch_dup_task_struct](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L244) 
1. 分配了新的内核堆栈. 如果启用了 `CONFIG_VMAP_STACK` , 内核将使用 [virtually mapped stacks](https://lwn.net/Articles/692208/) 防止内核堆栈溢出. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L525)
1. 任务的凭据已复制.  [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1628)
1. 通知调度程序有新任务派生.  [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1727) 
1. [task_fork_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L9063) 调用CFS调度程序类的方法.  此方法为当前正在运行的任务更新`vruntime`值(在[update_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L827) 内部完成) )并更新当前运行队列的 `min_vruntime` 值(内部 [update_min_vruntime](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L514)). 然后将 `min_vruntime` 值分配给分叉的任务-这样可以确保下一步提取该任务. 请注意, 目前还没有将新任务添加到`task_timeline`中. 
1. 许多不同的属性(例如, 有关文件系统, 打开的文件, 虚拟内存, 信号, 名称空间的信息)可以从当前任务中重用或复制. 通常根据`clone_flags`参数来决定是复制内容还是重用当前属性.  [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1731-L1765)
1. [copy_thread_tls](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1766) 被称为反过来又调用特定于体系结构 [copy_thread](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L254) 函数. 这个功能值得特别注意, 因为它可以作为 [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) RPi OS中的函数, 我想更深入地研究它. 

### copy_thread

整个功能在下面列出. 

```cpp
int copy_thread(unsigned long clone_flags, unsigned long stack_start,
        unsigned long stk_sz, struct task_struct *p)
{
    struct pt_regs *childregs = task_pt_regs(p);

    memset(&p->thread.cpu_context, 0, sizeof(struct cpu_context));

    if (likely(!(p->flags & PF_KTHREAD))) {
        *childregs = *current_pt_regs();
        childregs->regs[0] = 0;

        /*
         * Read the current TLS pointer from tpidr_el0 as it may be
         * out-of-sync with the saved value.
         */
        *task_user_tls(p) = read_sysreg(tpidr_el0);

        if (stack_start) {
            if (is_compat_thread(task_thread_info(p)))
                childregs->compat_sp = stack_start;
            else
                childregs->sp = stack_start;
        }

        /*
         * If a TLS pointer was passed to clone (4th argument), use it
         * for the new thread.
         */
        if (clone_flags & CLONE_SETTLS)
            p->thread.tp_value = childregs->regs[3];
    } else {
        memset(childregs, 0, sizeof(struct pt_regs));
        childregs->pstate = PSR_MODE_EL1h;
        if (IS_ENABLED(CONFIG_ARM64_UAO) &&
            cpus_have_const_cap(ARM64_HAS_UAO))
            childregs->pstate |= PSR_UAO_BIT;
        p->thread.cpu_context.x19 = stack_start;
        p->thread.cpu_context.x20 = stk_sz;
    }
    p->thread.cpu_context.pc = (unsigned long)ret_from_fork;
    p->thread.cpu_context.sp = (unsigned long)childregs;

    ptrace_hw_copy_thread(p);

    return 0;
}
```

您可能已经对其中的某些代码有些熟悉.  让我们更深入地研究它. 

```
struct pt_regs *childregs = task_pt_regs(p);
```

该函数从分配新的开始 [pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L119) 结构. 该结构用于提供对在`kernel_entry`中保存的寄存器的访问. 然后, 可以使用childregs变量准备新创建任务所需的任何状态. 如果任务然后决定转移到用户模式, 则状态将由`kernel_exit`宏恢复.  这里要了解的重要一点是 [task_pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L161) 宏不分配任何东西 - 它只是计算内核堆栈上的位置, 即 `kernel_entry` 存储寄存器, 对于新创建的任务, 该位置将始终位于内核堆栈的顶部. 

```
memset(&p->thread.cpu_context, 0, sizeof(struct cpu_context));
```

接下来, 清除分叉的任务 `cpu_context`. 

```
if (likely(!(p->flags & PF_KTHREAD))) {
```
  
然后进行检查以确定我们正在创建内核还是用户线程. 现在, 我们仅对内核线程情况感兴趣, 我们将在下一课中讨论第二种选择. 


  ```
  memset(childregs, 0, sizeof(struct pt_regs));
  childregs->pstate = PSR_MODE_EL1h;
  if (IS_ENABLED(CONFIG_ARM64_UAO) &&
      cpus_have_const_cap(ARM64_HAS_UAO))
          childregs->pstate |= PSR_UAO_BIT;
  p->thread.cpu_context.x19 = stack_start;
  p->thread.cpu_context.x20 = stk_sz;
  ```

  如果我们正在创建内核线程, 则将`cpu_context`的`x19`和`x20`寄存器设置为指向需要执行的函数(`stack_start`)及其参数(`stk_sz`).  将CPU切换到分叉任务后,  [ret_from_fork](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L942) 将使用这些寄存器跳转到所需的功能.  (我不太明白为什么我们还要在这里设置`childregs-> pstate`. `ret_from_fork`在跳转到存储在`x19`中的函数之前不会调用`kernel_exit`, 即使内核线程决定转到用户模式`childregs`将会被覆盖. 有什么想法吗？)
  
```
p->thread.cpu_context.pc = (unsigned long)ret_from_fork;
p->thread.cpu_context.sp = (unsigned long)childregs;
```

接下来的`cpu_context.pc`设置为`ret_from_fork`指针-这确保我们在第一次上下文切换后返回到`ret_from_fork`.  `cpu_context.sp`被设置为`childregs`的正下方. 我们仍然需要在堆栈顶部使用 `childregs`, 因为在内核线程完成执行之后, 任务将被移至用户模式, 并且将使用 `childregs` 结构. 在下一课中, 我们将详细讨论这种情况. 

就是关于`copy_thread`函数. 现在, 让我们回到派生过程中的位置. 

### Fork procedure (continued)

1. 在`copy_process`成功为分叉任务`_do_fork`准备好`task_struct`之后, 现在可以通过调用来运行它 [wake_up_new_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2438). 这个被完成在 [这](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2074). 然后任务状态更改为 `TASK_RUNNING` 和  [enqueue_task_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L4879) 调用CFS方法, 该方法触发执行 [__enqueue_entity](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L549) 实际上将任务添加到`task_timeline`红黑树中. 

1. 在 [这](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2463) 行, [check_preempt_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L871) 被调用, 依次调用 [check_preempt_wakeup](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L6167) CFS 方法. 此方法负责检查当前任务是否应被其他任务抢占. 这正是即将发生的事情, 因为我们刚刚在时间轴上放置了一个新任务, 而该任务的`vruntime`可能性极小.  所以 [resched_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L479) 函数被触发, 为当前任务设置 `TIF_NEED_RESCHED` 标志. 

1. `TIF_NEED_RESCHED` 在当前任务从异常处理程序退出之前进行检查( `fork`, `vfork`和`clone`都是系统调用, 每个系统调用都是一种特殊的异常类型. ) 可以查看[这里](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L801). 注意 [_TIF_WORK_MASK](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L109) 包括 ` _TIF_NEED_RESCHED` . 同样重要的是要了解, 在创建内核线程的情况下, 直到下一个定时器滴答或父任务挥发性地调用`schedule()`时, 才会启动新线程. 

1. 如果当前任务需要重新安排,  [do_notify_resume](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/signal.c#L744) 被触发, 依次调用 [schedule](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3418). 最终, 我们到达了触发任务计划的地步, 我们将在此点停止. 

### 结论

既然您了解了如何创建新任务并将其添加到调度程序, 现在该看看调度程序本身如何工作以及如何实现上下文切换了. 这是我们将在下一章中探讨的内容. 

##### 上一页

4.2 [流程调度程序：调度程序的基本结构](../../../docs/lesson04/linux/basic_structures.md)

##### 下一页

4.4 [流程调度程序：调度程序](../../../docs/lesson04/linux/scheduler.md)
