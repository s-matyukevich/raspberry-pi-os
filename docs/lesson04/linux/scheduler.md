## 4.4: Scheduler 

We have already learned a lot of details about the Linux schedule inner workings, so there is not so much left for us. To make the whole picture complete in this chapter we will take a look at 2 important scheduler entry points:

1. [scheduler_tick()](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3003) function, which  is called at each timer interrupt.
1. [schedule()](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3418) function, which is called each time when the current task needs to be rescheduled. 

The third major thing that we are going to investigate in this chapter is the concept of context switch. A context switch is the process that suspends the current task and runs another task instead - this process is highly architecture specific and closely correlates with what we have been doing when working with RPi OS.

### scheduler_tick

This function is important for 2 reasons:

1. It provides a way for the scheduler to update time statistics and runtime information for the current task. 
1. Runtime information then is used to determine whether the current task needs to be preempted, and if so `schedule()` function is called.

As well as most of the previously explored functions, `scheduler_tick` is too complex to be fully explained - instead, as usual, I will just highlight the most important parts.

1. The main work is done inside CFS method [task_tick_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L9044). This method calls [entity_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3990) for the `sched_entity` corresponding to the current task. When looking at the source code, you may be wondering why instead of just calling `entry_tick` for the current `sched_entry`, `for_each_sched_entity` macro is used instead?  `for_each_sched_entity` doesn't iterate over all `sched_entry` in the system. Instead, it only traverses the `sched_entry`  inheritance tree up to the root. This is useful when tasks are grouped - after updating runtime information for a particular task, `sched_entry` corresponding to the whole group is also updated.

1. [entity_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3990) does 2 main things:
  * Calls [update_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L827), which is responsible for updating task's `vruntime` as well as runqueue's `min_vruntime`. An important thing to remember here is that `vruntime` is always based on 2 things: how long task has actually been executed and tasks priority.
  * Calls [check_preempt_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3834), which checks whether the current task needs to be preempted. Preemption happens in 2 cases:
    1. If the current task has been running for too long (the comparison is made using normal time, not `vruntime`). [link](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3842)
    1. If there is a task with smaller `vruntime` and the difference between `vruntime` values is greater than some threshold. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3866)
    
    In both cases the current task is marked for preemption by calling [resched_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L479) function.

We have already seen in the previous chapter how calling `resched_curr` leads to `TIF_NEED_RESCHED` flag being set for the current task and eventually `schedule` being called.

That's it about `schedule_tick` now we are finally ready to take a look at the `schedule` function.

### schedule

We have already seen so many examples were `schedule` is used, so now you are probably anxious to see how this function actually works. You will be surprised to know that the internals of this function are rather simple.

1. The main work is done inside [__schedule](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3278)  function. 
1. `__schedule` calls [pick_next_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3199) which redirect most of the work to the [pick_next_task_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L6251) method of the CFS scheduler.
1. As you might expect `pick_next_task_fair` in a normal case just selects the leftmost element from the red-black tree and returns it. It happens [here](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3915).
1. `__schedule` calls [context_switch](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2750), which does some preparation work and calls architecture specific [__switch_to](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L348) function, were low-level arch specific task parameters are prepared to the switch.
1. `__switch_to` first switches some additional task components, like, for example, TLS (Thread-local Store) and saved floating point and NEON registers.
1. Actual switch takes place in the assembler [cpu_switch_to](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L914) function. This function should be already familiar to you because I copied it almost without any changes to the RPi OS. As you might remember, this function switches callee-saved registers and task stack. After it returns, the new task will be running using its own kernel stack.

### Conclusion

Now we are done with the Linux scheduler. The good thing is that it appears to be not soo difficult if you focus only on the very basic workflow. After you understand the basic workflow you probably might want to to make another path through the schedule code and pay more attention to the details, because there are so many of them. But for now, we are happy with our current understanding and ready to move to the following lesson, which describes user processes and system calls.

##### Previous Page

4.3 [Process scheduler: Forking a task](../../../docs/lesson04/linux/fork.md)

##### Next Page

4.5 [Process scheduler: Exercises](../../../docs/lesson04/exercises.md)
