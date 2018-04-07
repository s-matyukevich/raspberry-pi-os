## 4.3: Forking a task

Scheduling is all about selecting a proper task to run from the list of available tasks. But before the scheduler will be able to do its job we need to somehow fill this list. The way in which new tasks can be created is the main topic of this chapter. 

For now, we want to focus only on kernel threads and postpone the discussion of user-mode functionality till the next lesson. However, not everywhere it would be possible, so be prepared to learn a little bit about executing tasks in user mode as well.

### Init task

When the kernel is started there is a single task running: init task. The corresponding `task_struct` is defined [here](https://github.com/torvalds/linux/blob/v4.14/init/init_task.c#L20) and is initialized by [INIT_TASK](https://github.com/torvalds/linux/blob/v4.14/include/linux/init_task.h#L226) macro. This task is critical for the system because all other tasks in the system are derived from it.

### Creating new kernel threads

In Linux it is not possible to create a new task from scratch - instead, all tasks are forked from a currently running task. Now, as we've seen from were the initial task came from, we can try to explore how new tasks can be created from it. 

There are 4 ways in which a new task can be created.

1. [fork](http://man7.org/linux/man-pages/man2/fork.2.html) system call creates a full copy of the current process, including its virtual memory and it is used to create new processes (not threads). This syscall is defined [here](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2116)
1. [vfork](http://man7.org/linux/man-pages/man2/vfork.2.html) system call is similar to `fork` but it differs in that the child reuses parent virtual memory as well as stack, and the parent is blocked until the child finished execution. The definition of this syscall can be found [here](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2128)
1. [clone](http://man7.org/linux/man-pages/man2/clone.2.html) system call is the most flexible one - it also copies the current task but it allows to customize the process using `flags` parameter and allows to configure the entry point for the child task. In the next lesson, we will see how `glibc` clone wrapper function is implemented - this wrapper allows to use `clone` syscall to create new threads. 
1. Finally, [kernel_thread](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2109) function can be used to create new kernel threads. 

All of the above functions calls [_do_fork](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2020), which accept the following arguments.

* `clone_flags` Flags are used to configure fork behavior. The complete list of the flags can be found [here](https://github.com/torvalds/linux/blob/v4.14/include/uapi/linux/sched.h#L8)
* `stack_start` In case of `clone` syscall this parameter indicates the location of the user stack for the new task. If 'kernel_thread' calls `_do_fork` this parameter points to the function that needs to be executed in a kernel thread.
* `stack_size` In `arm64` architecture this parameter is only used in `kernel_thread` case - it is a pointer to the argument that needs to be passed to the kernel thread function. (And yes, I also find the naming of the last two parameters misleading)
* `parent_tidptr` `child_tidptr` Those 2 parameters are used only in `clone` syscall. Fork will store the child thread ID at the location `parent_tidptr` in the parent's memory, or it can store parent's ID at `child_tidptr`location.
* `tls`  [Thread Local Storage](https://en.wikipedia.org/wiki/Thread-local_storage)

### Fork procedure

Next, I want to highlight the most important events that take place during `_do_fork` execution, preserving their order.

1. [_do_fork](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2020) calls [copy_process](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1539)  `copy_process` is responsible for configuring new `task_struct`.
1. `copy_process` calls [dup_task_struct](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L512), which allocates new `task_struct` and copies all fields from the original one. Actual copying takes place in the architecture-specific [arch_dup_task_struct](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L244) 
1. New kernel stack is allocated. If `CONFIG_VMAP_STACK` is enabled the kernel uses [virtually mapped stacks](https://lwn.net/Articles/692208/) to protect against kernel stack overflow. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L525)
1. Task's credentials are copied. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1628)
1. The scheduler is notified that a new task is forked. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1727) 
1. [task_fork_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L9063) method of the CFS scheduler class is called. This method updates `vruntime` value for the currently running task (this is done inside [update_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L827) function) and updates `min_vruntime` value for the current runqueue (inside [update_min_vruntime](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L514)). Then `min_vruntime` value is assigned to the forked task - this ensures that this task will be picked up next. Note, that at this point of time new task still hasn't been added to the `task_timeline`.
1. A lot of different properties, such as information about filesystems, open files, virtual memory, signals, namespaces, are either reused or copied from the current task. The decision whether to copy something or reuse current property is usually made based on the `clone_flags` parameter. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1731-L1765)
1. [copy_thread_tls](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1766) is called which in turn calls architecture specific [copy_thread](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L254) function. This function deserves a special attention because it works as a prototype for the [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) function in the RPi OS, and I want to investigate it deeper.

### copy_process

The whole function is listed below.

```
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

Some of this code can be already a little bit familiar to you. Let's dig dipper into it.

```
struct pt_regs *childregs = task_pt_regs(p);
```

The function starts with allocating new [pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L119) struct. This struct s used to provide access to the registers, saved during `kernel_entry`. `childregs` variable then can be used to prepare whatever state we need for the newly created task. If the task then decides to move to user mode the state will be restored by the `kernel_exit` macro. An important thing to understand here is that [task_pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L161) macro doesn't allocate anything - it just calculate the position on the kernel stack, were `kernel_entry` stores registers, and for the newly created task, this position will always be at the top of the kernel stack.

```
memset(&p->thread.cpu_context, 0, sizeof(struct cpu_context));
```

Next, forked task `cpu_context` is cleared.

```
if (likely(!(p->flags & PF_KTHREAD))) {
```
  
Then a check is made to determine whether we are creating a kernel or a user thread. For now, we are interested only in kernel thread case and we will discuss the second option in the next lesson.

  ```
  memset(childregs, 0, sizeof(struct pt_regs));
  childregs->pstate = PSR_MODE_EL1h;
  if (IS_ENABLED(CONFIG_ARM64_UAO) &&
      cpus_have_const_cap(ARM64_HAS_UAO))
          childregs->pstate |= PSR_UAO_BIT;
  p->thread.cpu_context.x19 = stack_start;
  p->thread.cpu_context.x20 = stk_sz;
  ```

  If we are creating a kernel thread `x19` and `x20` registers of the `cpu_context` are set to point to the function that needs to be executed (`stack_start`) and its argument (`stk_sz`). After CPU will be switched to the forked task [ret_from_fork](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L942) will use those registers to jump to the needed function. (I don't quite understand why do we also need to set `childregs->pstate` here. `ret_from_fork` will not call `kernel_exit` before jumping to the function stored in `x19`, and even if the kernel thread decides to move to the user mode `childregs` will be overwritten anyway. Any ideas?)
  
```
p->thread.cpu_context.pc = (unsigned long)ret_from_fork;
p->thread.cpu_context.sp = (unsigned long)childregs;
```

Next `cpu_context.pc` is set to `ret_from_fork` pointer - this ensures that we return to the `ret_from_fork` after the first context switch. `cpu_context.sp` is set to the location just below the `childregs` We still need `childregs` at the top of the stack because after the kernel thread finishes its execution the task will be mowed to user mode and `childregs` structure will be used. In the next lesson, we will discuss in details how this happens.

That's it about `copy_thread` function. Now let's return to the place in the fork procedure from where we left.

### Fork procedure (continued)

1. After `copy_process` succsesfully prepares `task_struct` for the forked task `_do_fork` can now run it by calling [wake_up_new_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2438) This is done [here](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2074). Then task state is changed to `TASK_RUNNING` and  [enqueue_task_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L4879) CFS method is called, wich triggers execution of the [__enqueue_entity](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L549) that actually adds task to the `task_timeline` red-black tree.

1. At [this](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2463) line [check_preempt_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L871) is called, which in its turn calls [check_preempt_wakeup](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L6167) CFS method. This method is responsible for checking whether the current task should be preempted by some other task. That is exactly what is going to happen because we have just put a new task on the timeline that has minimal possible `vruntime`. So [resched_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L479) function is triggered, which sets `TIF_NEED_RESCHED` flag for the current task.

1. `TIF_NEED_RESCHED` is checked just before the current task exit from an exception handler (`fork`, `vfork` and `clone` are all system call, and each system call in a special type of exception) The check is made [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L801) Note that [_TIF_WORK_MASK](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L109) includes `_TIF_NEED_RESCHED`. It is also important to understand that in case of a kernel thread creation, the new thread will not be started until the next timer tick or until the parent task volatirely calls `schedule()`

1. If the current task needs to be rescheduled [do_notify_resume](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/signal.c#L744) is triggered, which in turn calls [schedule](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3418) Finally we reached the point where task scheduling is triggered, and we are going to stop at this point.

### Conclusion

Now, when you understand how new tasks are created and added to the scheduler, it is time to take a look on how the scheduler itself works and how context switch is implemented - that is something we are going to explore in the next chapter.

