## Lesson 4: Forkin a task

Scheduler is all about selection proper task to run from  the list of all available tasks. But before scheduler will be able to do its job we need to somehow fill this list. The way in wich new tasks can be created is the main topic of this cahpter. 

For now we want to focus only on kernel threads and postpone the discussion of user related funcitonality till the next lesson. However, not everywhere it would be posible, so be prepared to learn a little bit about executing tasks in user mode right away.

### Init task

When the kernel is started there is a single task running: init task. The corresonding `task_struct` is defined [here](https://github.com/torvalds/linux/blob/v4.14/init/init_task.c#L20) and is initialized by [INIT_TASK](https://github.com/torvalds/linux/blob/v4.14/include/linux/init_task.h#L226) macro. This task is very important, because all other tasks in the system are derived from it.

### Creating new kernel threads

In linux it is not posible to create a new task from schatch - instead all tasks are forked from the currently running task. Now, as we've seen from were the initial task came from, we can try to explore how new tasks can be created from it. 

There are 4 ways in wich a new task can be created.

1. [fork](http://man7.org/linux/man-pages/man2/fork.2.html) system call creates a full copy of the current process, including its virtual memory and it is used to create new processes (not threads). This syscall is defined [here](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2116)
1. [vfork](http://man7.org/linux/man-pages/man2/vfork.2.html) system call is similar to `fork` but it differs in that the child reuses parent virtual memory as well as stack and the parent is blocked untill the child finished execution. The definition of this syscall can be found [here](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2128)
1. [clone](http://man7.org/linux/man-pages/man2/clone.2.html) system call is the most flexible one - it also copies the current task but it allows to customize the process using `flags` parameter and allows to configure the starting point for the child. In the next lesson we will see how glibc clone wrapper function is implemented - this wrapper allows to use `clone` syscall to create new threads. 
1. Finally, [kernel_thread](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2109) function can be used to create new kernel threads. o

All of the above functions calls [_do_fork](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2020), wich accept the following arguments.

* `clone_flags` - flags are used to configure fork behaviour. The complete list of the flags can be found [here](https://github.com/torvalds/linux/blob/v4.14/include/uapi/linux/sched.h#L8)
* `stack_start` - in case of `clone` syscall this parameter indicates the location of the new user stack for clonned task. If 'kernel_thread' calls `_do_fork` this parameter points to the function that need to be executed in a kernel thread.
* `stack_size` - in `arm64` architecture this parameter is only used in `kernel_thread` case - it is a pointer to the argument that need to be passed to the kernel therad function. (And yes, I also find the naming for the last two parameters missleading)
* `parent_tidptr` `child_tidptr` - those 2 parameters are used only in `clone` syscall. Fork will store the child thread ID at the location `parent_tidptr` in the parent's memory, of it can store parent's ID at `child_tidptr`location.
* `tls` - [Thread Local Storage](https://en.wikipedia.org/wiki/Thread-local_storage)

Next, I want to highlite the most important events that take place during `_do_fork` eexecution, preserving there order.

1. [_do_fork](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2020) calls [copy_process](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1539)  `copy_process` is responsible for configuring new `task_struct`.
1. `copy_process` calls [dup_task_struct](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L512), wich allocates new `task_struct` and copies all fields from the original one. Actual copying takes place in the architecture-specific [arch_dup_task_struct](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L244) 
1. New kernel stack is alocated. If `CONFIG_VMAP_STACK` is enabled the kernel uses [virtually mapped stacks](https://lwn.net/Articles/692208/) to protect agains kernel stack overflow. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L525)
1. Task's credentials are copied. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1628)
1. Scheduler is notified that a new task is forked. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1727) 
1. [task_fork_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L9063) metod of the CFS scheduler class is called. This method updates `vruntime` value for the currently running task (this is done inside [update_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L827) function) and updates `min_vruntime` value for the current runqueue (inside [update_min_vruntime](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L514)). Then `min_vruntime` value is assigned to the forked task - this ensures that this task will be picked up next. Note, that at this point of time new task stil hasn't beed added to the `task_timeline`.
1. A lot of different properties, such as information about filesystes, open files, virtual memory, signals, namespaces, are either reused or copied from the curren task. The deceigion whether to copy somthing or reuse current property is usually made based on the `clone_flags` parameter. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1731-L1765)
1. [copy_thread_tls](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L1766) is called wich in turn calls architecture specific [copy_thread](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L254) function. This function deserves a special attension, because it works as a prototype for the [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) function in RPi OS, and I want to investigate it deeper.

  ```
  struct pt_regs *childregs = task_pt_regs(p);
  ```

  The function starts with allocating new [pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L119) struct. This struct s used to provide access to the registers, saved during `kernel_entry`. `childregs` variable then can be used to prepare whatever state we need for the newely created task. If the task then desides to move to user mode this state will be restored by the `kernel_exit` macro. Important thing to understand here is that [task_pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L161) macro doesn't actually allocate anything - it just calculate the position on the kernel stack, were `kernel_entry` stores  registers, and for newely created task this position will always be at the top of the kernel stack.

  ```
  memset(&p->thread.cpu_context, 0, sizeof(struct cpu_context));
  ```

  Next, forked task `cpu_context` is cleared.

  ```
  if (likely(!(p->flags & PF_KTHREAD))) {
  ```
  
  Next, a check is made to determing whether we are creating a kernel thread or a user therd. For now we are interested only in kernel thread case and we will discuss the second option in the next lesson.

  ```
  memset(childregs, 0, sizeof(struct pt_regs));
  childregs->pstate = PSR_MODE_EL1h;
  if (IS_ENABLED(CONFIG_ARM64_UAO) &&
      cpus_have_const_cap(ARM64_HAS_UAO))
          childregs->pstate |= PSR_UAO_BIT;
  p->thread.cpu_context.x19 = stack_start;
  p->thread.cpu_context.x20 = stk_sz;
  ```

  If we are creating a kernel thread `x19` and `x20` registers of the `cpu_context` are set to point to the function that need to be executed (`stack_start` srgument) and its argument (`stk_sz`). After CPU will be switched to the forked task [ret_from_fork](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L942) will used those registers to jump to the needed function. (I don't quite understand why do we also need to set `childregs->pstate` here. `ret_from_fork` will not call `kernel_exit` before jumping to the kernel thread function, and even if the kernel thread decides to move to the user mode `childregs` will be overriten anyway. Any ideas?)
  
  ```
  p->thread.cpu_context.pc = (unsigned long)ret_from_fork;
  p->thread.cpu_context.sp = (unsigned long)childregs;
  ```

  Next `cpu_context.pc` is set to `ret_from_fork` pointer - this ensures that we return to the `ret_from_fork` after context switch. `cpu_context.sp` is set to the location just below the `childregs` - after kernel thread finishes its execution the task will be mowed to user mode and `childregs` structure will be used. In the next lesson we will discuss in details how this happens.

  That's it about `copy_thread` function. 

1. After `copy_process` succsesfully prepares `task_struct` for the forked task `_do_fork` can now run it by calling [wake_up_new_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2438) This is done [here](https://github.com/torvalds/linux/blob/v4.14/kernel/fork.c#L2074). Then task state is changed to `TASK_RUNNING` and  [enqueue_task_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L4879) CFS method is called, wich triggers execution of the [__enqueue_entity](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L549) that actually adds task to the `task_timeline` red-black tree.

1. At [this](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2463) line [check_preempt_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L871) is called, wich in its turn calls [check_preempt_wakeup](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L6167) CFS method. This method is responsible for checking whether current task should be preempted by some other task. That is exactly what is going to happen, because we have just put a new task on the timeline that has minimal posible `vruntime`. So [resched_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L479) function is triggered, wich sets `TIF_NEED_RESCHED` flag for the current task.

1. `TIF_NEED_RESCHED` is checked just before current task exit from an exception handler (`fork`, `vfork` and `clone` are all system call, and each system call in a special type of exception) The check is made [here](). This check is made [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L801) . Note that [_TIF_WORK_MASK](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L109) includes `_TIF_NEED_RESCHED`. It is also important to understand that in case of a kernel thread creation, new thread will not be started untill next timer tick or untill parent volatirely calls `schedule()`

1. If current task need to be rescheduled [do_notify_resume](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/signal.c#L744) wich in turn calls [schedule](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3418), wich is the main scheduler function. 

### Conclusion

Now, when you understand how new tasks are created and added to the scheduler it is time to take a look on how the cheduler works and how context switch is implemented - that is somthing we are going to explore  in the next chapter.
