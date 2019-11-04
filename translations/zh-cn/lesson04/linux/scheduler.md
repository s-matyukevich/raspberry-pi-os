## 4.4: Scheduler

我们已经了解了许多有关Linux调度程序内部工作原理的详细信息，因此我们所剩无几。为了使本章更加完整，我们将介绍两个重要的调度程序入口点：

1. [scheduler_tick()](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3003) 函数，在每个定时器中断时调用。
1. [schedule()](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3418) 函数，每次需要重新安排当前任务时调用。

本章要研究的第三项主要内容是上下文切换的概念。上下文切换是挂起当前任务并运行另一个任务的过程-该过程是高度特定于体系结构的，并且与我们使用RPi OS时所做的工作紧密相关。

### scheduler_tick

此功能很重要，原因有两个：

1. 它为调度程序提供了一种更新当前任务的时间统计信息和运行时信息的方法。
1. 然后，运行时信息用于确定是否需要抢占当前任务，如果是，则调用`schedule()`函数。

与大多数以前探索的功能一样，`scheduler_tick` 太复杂而无法完全解释-相反，像往常一样，我将仅强调最重要的部分。

1. 主要工作在CFS方法内部完成 [task_tick_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L9044). 该方法调用 [entity_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3990) 对应于当前任务的 `sched_entity` 。在查看源代码时，您可能想知道为什么不使用当前的`sched_entry`来调用`entry_tick`而是使用`for_each_sched_entity`宏呢？ `for_each_sched_entity`不会遍历系统中的所有`sched_entry`。相反，它仅遍历`sched_entry`继承树直到根。当对任务进行分组时，这很有用 - 在更新特定任务的运行时信息后，也会更新与整个组相对应的`sched_entry`。

1. [entity_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3990) 做两件事:
  * 调用 [update_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L827), 它负责更新任务的 `vruntime` 和运行队列的 `min_vruntime`。这里要记住的重要一点是，`vruntime`始终基于两件事：实际执行任务多长时间和任务优先级。
  * 调用 [check_preempt_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3834), 检查当前任务是否需要抢占。抢占发生在2种情况下：
    1. 如果当前任务已经运行了太长时间（比较使用的是正常时间，而不是`vruntime`）。 [link](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3842)
    1. 如果任务的`vruntime`较小，并且`vruntime`值之间的差异大于某个阈值。 [link](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3866)

    在这两种情况下，当前任务都通过调用 [resched_curr](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L479) 函数来标记为抢占.

我们已经在上一章中看到了如何调用`resched_curr`导致为当前任务设置`TIF_NEED_RESCHED`标志，并最终调用`schedule`。

就是关于 `schedule_tick` 的事情了，现在我们终于可以看一下 `schedule` 功能了。

### schedule

我们已经看到了很多使用`schedule`的示例，因此现在您可能急于了解此函数的实际工作方式。您会惊讶地知道此函数的内部非常简单。


1. 主要工作在内部 [__schedule](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3278)  函数完成.
1. `__schedule` 调用 [pick_next_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L3199) 将大部分工作重定向到 [pick_next_task_fair](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L6251) CFS调度程序的方法。
1. 如您所料 `pick_next_task_fair` 通常情况下，只需从红黑树中选择最左边的元素并返回它即可。 它在 [这里](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/fair.c#L3915).
1. `__schedule` calls [context_switch](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/core.c#L2750), 做一些准备工作并调用特定于体系结构 [__switch_to](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/process.c#L348) 函数, 在其中为交换机准备了低级别的特定于`Arch`的任务参数。
1. `__switch_to` 首先切换一些其他任务组件，例如TLS（线程本地存储）和已保存的浮点和NEON寄存器。
1. 实际切换发生在汇编 [cpu_switch_to](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/entry.S#L914) 函数. 您应该已经熟悉此功能，因为我几乎没有复制RPi OS就复制了它。您可能还记得，此函数切换被调用者保存的寄存器和任务堆栈。返回后，新任务将使用其自己的内核堆栈运行。

### 结论

现在我们完成了Linux调度程序。好处是，如果您只关注最基本的工作流程，似乎并不那么困难。了解基本的工作流程后，您可能需要在计划代码中另辟蹊径，并更加注意细节，因为其中有很多。但就目前而言，我们对当前的理解感到满意，并准备继续进行下一课，该课描述了用户流程和系统调用。

##### 上一页

4.3 [流程计划程序：分派任务](../../../docs/lesson04/linux/fork.md)

##### 下一页

4.5 [流程计划程序：练习](../../../docs/lesson04/exercises.md)
