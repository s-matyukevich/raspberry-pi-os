## Lesson 4: Scheduler 

We have already seem a lot of details about inner workings of the linux scheduler, so there is not so much left for us. To make the whole picture complete in this chapter we will take a look on 2 important scheduler entrypoints

1. [scheduler_tick()](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3003) function, wich  is called at each timer interrupt.
1. [schedule()](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3418) function, wich is called each time when the current task need to be rescheduled. 

The third major thing that we are going to taks a look in this chapter is the concept of context switch. Context switch is the process wich suspend current task and runs another task instead - this process is highly architecture specific and closely corelates with what we has beed doing when working on RPi OS.

### scheduler_tick

This function is important for 2 reasons:

1. It provides a way for the scheduler to update time statistics and runtime information for the current task. 
1. Runtimer information then is used to determine whether current task need to be preempted, and if so `schedule` function is called.

As well as most of the previously explored functions, `scheduler_tick` is too complex to be fully explien - instead, as ususal, I will just highlight only the most important parts.

1. The main work is done inside CFS method [task_tick_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L9044). This method calls [entity_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3990) for the `sched_entity` corresponding to the current task. When looking at the source code you may be wondering why instead of just calling `entry_tick` for the current `sched_entry`, `for_each_sched_entity` macro is used instead?  `for_each_sched_entity` doesn't iterate over all `sched_entry` in the system. Instead it only traverses the tree of `sched_entry` inheritance up. This is usefull when tasks are gouped - after updating runtime information for a particular task, `sched_entry` corresponding to the whole group is also updated.

1. [entity_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3990) does 3 main things
  * Calls [update_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L827), wich is responsible for updating task's `vruntime` as well as runqueue'a `min_vruntime`. Important thing to remember here is `vruntime` is always based on 2 things: how long task has actually been executed and tasks priority.
  * Calls [check_preempt_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3834), wich checks whether current task need to be preempted. Preemption happens in 2 cases
    1. If the current task has benn running for too long (the comparison is made using normal time, not `vruntime`) [link](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3842)
    1. If  there is a task with smaller `vruntime` and the difference between `vruntime` values is greater than some threshoald. [link](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3866)
    In both cases current task is marked for preemption by calling [resched_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L479) function.

1. We have already seen in the previous chapter how calling `resched_curr` leads to `TIF_NEED_RESCHED` blag being set for the current task and eventually `schedule` being called.

That's it about `schedule_tick` now we are finally ready to take a look at the `schedule` function.

### schedule

We have already seen so many examples were `schedule` is used, so now you are probably anctios to see how this function actually works. You will be supprised to see  that the internals of this function are rather simple

1. The main work is done inside [__schedule](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3278)  function. 
1. `__schedule` calls [pick_next_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3199) wich redirect most of the work to the [pick_next_task_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L6251) method of the CFS scheduler.
1. As you might expect `pick_next_task_fair` in a normal case fust selects the leftmost element from the red-black tree and returns it. It happens [here](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3915)
1. `__schedule` calls [context_switch](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2750), wich does some preparation work and calls architecture specific [__switch_to](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L348) were low level arch specific task parameters are prepared to the switch.
1. `__switch_to` first switches some additional task parameters, like TLS (Thread local Store) location of saved floating point and NEON registers.
1. Actual switch taks place in the assembler [cpu_switch_to](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L914) function. This function should be already familiar to you, because I copied it almost withount ane changes to the RPi OS. As a remider, this function switches calee-saved registers and task stack. After it returns new task will be running using its own kernel stack.

### Conclusion

Now we are done with the linux scheduler. The good thing is that it appears to be not soo difficult, if you focus only on the very basic workflow. After you understand the basic workflow you probably might want to to make another path through the schedule code and pay more attension to the  details, because there are so many of them. But for now we are happy with aou curent understanding and ready to move to the following lesson, wich describes user processes and system calls.
