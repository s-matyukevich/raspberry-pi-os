## 4.2: Scheduler basic structures
 
In all previous lessons we have been working mostly with either architecture specific code or driver code, and now it is the first time we will dig deep into the core of the Linux kernel. This task isn't simple, and it requires some preparations: before you will be able to understand the Linux schedule source code, you need to become familiar with a few major concepts that the schedule is based on.

### [task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L519)

This is one of the most critical structures in the whole kernel — it contains all information about a running task. We already briefly touched `task_struct`  in lesson 2 and we even have implemented our own `task_struct` for the RPi OS, so I assume that by this time you should already have a basic understanding how it is used. Now I want to highlight a few important fields of this struct that are relevant to our discussion.

* [thread_info](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L525) This is the first field of the `task_struct` and it contains all fields that must be accessed by the low-level architecture code. We have already seen how this happens in lesson 2 and will encounter a few other examples later. [thread_info](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L39) is architecture specific. In `arm64` case, it is a simple structure with a few fields.
  ```
  struct thread_info {
          unsigned long        flags;        /* low level flags */
          mm_segment_t        addr_limit;    /* address limit */
  #ifdef CONFIG_ARM64_SW_TTBR0_PAN
          u64            ttbr0;        /* saved TTBR0_EL1 */
  #endif
          int            preempt_count;    /* 0 => preemptable, <0 => bug */
  };
  ```
  `flags` field is used very frequently — it contains information about the current task state (whether it is under a trace, whether a signal is pending, etc.) All possible flags values can be found [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L79)
* [state](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L528) Task current state (whether it is currently running, waiting for an interrupt, exited etc.) All possible task states are described [here](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L69)
* [stack](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L536) When working on the RPi OS, we have seen that `task_struct` is always kept at the bottom of the task stack, so we can use a pointer to `task_struct` as a pointer to the stack. Kernel stacks have constant size, so finding stack end is also an easy task. I think that the same approach was used in the early versions of the Linux kernel, but right now, after the introduction of the [vitually mapped stacks](https://lwn.net/Articles/692208/), `stack` field is used to store a pointer to the kernel stack.
* [thread](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L1108) Another important architecture specific structure is [thread_struct](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L81). It contains all information (such as [cpu_context](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L65)) that is used during a context switch. In fact, the RPi OS implements its own `cpu_context` that is used exactly in the same way as the original one.
* [sched_class and sched_entity](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L562-L563) Those fields are used in schedule algorithm - more on them follows.

### Scheduler class

In Linux, there is an extendable mechanism that allows each task to use its own scheduling algorithm. This mechanism uses a structure [sched_class](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1400). You can think about this structure as an interface that defines all methods that a schedules class have to implement. Let's see what kind of methods are defined in the `sched_class` interface. (Not all of the methods are shown, but only those that I consider the most important for us)

* [enqueue_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1403) is executed each time a new task is added to a scheduler class.
* [dequeue_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1404) is called when a task can be removed from the scheduler.
* [pick_next_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1418) Core scheduler code calls this method when it needs to decide which task to run next.
* [task_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1437) is called on each timer tick and gives the scheduler class an opportunity to measure how long the current task is being executed and notify core scheduler code if it needs to be preempted.

There are a few `sched_class` implementations. The most commonly used one, which is typically used for all user tasks, is called "Completely Fair Scheduler (CFS)"

### Completely Fair Scheduler (CFS)

The principals behind CFS algorithm are very simple: 
1. For each task in the system, CFS measures how much CPU time has been already allocated to it (The value is stored in [sum_exec_runtime](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L385) field of the `sched_entity` structure) 
1. `sum_exec_runtime` is adjusted accordingly to task priority and saved in [vruntime](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L386) field.
1. When CFS need to pick a new task to execute, the one with the smallest `vruntime` is selected.

Linux scheduler uses another important data structure that is called "runqueue" and is described by the [rq](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L667) struct. There is a single instance of a runqueue per CPU. When a new task needs to be selected for execution, the selection is made only from the local runqueue. But if there is a need, tasks can be balanced between different `rq` structures. 

Runqueues are used by all scheduler classes, not only by CFS. All CFS specific information is kept in [cfs_rq](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L420) struct, wich is embedded in the `rq` struct. One important field of the `cfs_rq` struct is called [min_vruntime](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L425) — this is the lowest `vruntime` from all tasks, assigned to a runqueue. `min_vruntime` is assigned to a newly forked task — this ensures that the task will be selected next, because CFS always ensures that a task with the smallest `vruntime` is picked. This approach so ensures that the new task will not be running for an unreasonably long time before it will be preempted.

All tasks, assigned to a particular runqueue and tracked by CFS are kept in [tasks_timeline](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L430) field of the `cfs_rq` struct. `tasks_timeline` represents a [Red–black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree), which can be used to pick tasks ordered by their `vruntime` value. Red-black trees have an important property: all operations on it (search, insert, delete) can be done in [O(log n)](https://en.wikipedia.org/wiki/Big_O_notation) time. This means that even if we have thousands of concurrent tasks in the system all scheduler methods still executes very quickly. Another important property of a red-black tree is that for any node in the tree its right child will always have larger `vruntime` value than the parent, and left child's `vruntime` will be always less or equal then the parent's `vruntime`. This has an important implication: the leftmost node is always the one with the smallest `vruntime`.

### [struct pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L119) 

I have already shown you how all general purpose registers are saved on the stack when an interrupt is taken — we explored how this process works in Linux kernel and implemented a similar one for the RPi OS. What I haven't mentioned yet, is that it is entirely legal to manipulate those registers while processing an interrupt. It is also legal to manually prepare a set of registers and put them on the stack — this is done when a kernel thread is moved to a user mode, and we are going to implement the same functionality in the next lesson. For now, you just need to remember that `pt_regs` structure is used to describe saved registers and it must have its fields ordered in the same order in with register are saved in the `kernel_entry` macro. In this lesson, we are going to see a few examples of how this structure is used.

### Conclusion

There are far more important structures, algorithms and concepts related to scheduling, but what we've just explored is a minimal set, required to understand the next two chapters. Now it's time to see how all of this actually works.

##### Previous Page

4.1 [Process scheduler: RPi OS Scheduler](../../../docs/lesson04/rpi-os.md)

##### Next Page

4.3 [Process scheduler: Forking a task](../../../docs/lesson04/linux/fork.md)
