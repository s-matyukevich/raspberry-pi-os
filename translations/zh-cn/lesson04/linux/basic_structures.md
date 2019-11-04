## 4.2: 调度程序的基本结构

在先前的所有课程中，我们主要使用体系结构特定代码或驱动程序代码，现在这是我们第一次深入研究Linux内核的核心。该任务并不简单，需要做一些准备工作：在能够理解Linux调度程序源代码之前，您需要熟悉调度程序所基于的一些主要概念。

### [task_struct](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L519)

这是整个内核中最关键的结构之一-它包含有关正在运行的任务的所有信息。我们已经在第2课中简要介绍了 `task_struct` ，甚至为RPi OS实现了自己的 `task_struct` ，因此我认为到那时您应该已经对如何使用它有了一个基本的了解。现在，我想强调该结构中与我们的讨论相关的一些重要领域。

* [thread_info](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L525) 这是 `task_struct` 的第一个字段，它包含所有必须由低级架构代码访问的字段。我们已经在第2课中看到了这是如何发生的，以后还会遇到其他一些示例。 [thread_info](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L39) 是特定于体系结构的。在 `arm64` 情况下，它是一个具有几个字段的简单结构。

  ```cpp
  struct thread_info {
          unsigned long        flags;        /* low level flags */
          mm_segment_t        addr_limit;    /* address limit */
  #ifdef CONFIG_ARM64_SW_TTBR0_PAN
          u64            ttbr0;        /* saved TTBR0_EL1 */
  #endif
          int            preempt_count;    /* 0 => preemptable, <0 => bug */
  };
  ```

  `flags` 字段使用非常频繁-它包含有关当前任务状态的信息（是否处于跟踪状态，信号是否挂起等）。可以找到所有可能的标志值 [这里](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/thread_info.h#L79).
* [state](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L528) 任务当前状态 (它是否正在运行，正在等待中断，退出等。). 描述了所有可能的任务状态在 [这里](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L69).
* [stack](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L536) 在RPi OS上工作时，我们已经看到`task_struct`始终位于任务堆栈的底部，因此我们可以使用指向`task_struct`的指针作为指向堆栈的指针。内核堆栈的大小恒定，因此找到堆栈末端也是一项容易的任务。 我认为在Linux内核的早期版本中使用了相同的方法，但是现在，在引入Linux内核之后的[虚拟映射堆栈 vitually mapped stacks](https://lwn.net/Articles/692208/), `stack` 字段用于存储指向内核堆栈的指针。
* [thread](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L1108) 另一个重要的体系结构特定结构是 [thread_struct](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L81). 它包含所有信息 (如 [cpu_context](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/processor.h#L65)) 在上下文切换期间使用。实际上，RPi OS实现了自己的`cpu_context`，其使用方式与原始方式完全相同。
* [sched_class and sched_entity](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L562-L563) 这些字段用于调度算法 - 关于它们的更多内容如下。

### 调度程序类

在Linux中，有一个可扩展的机制，该机制允许每个任务使用其自己的调度算法。该机制使用一种结构 [sched_class](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1400). 您可以将此结构视为定义了`schedules类`必须实现的所有方法的接口。让我们看看在`sched_class`接口中定义了哪种方法。 （未显示所有方法，仅显示了我认为对我们最重要的方法）

* [enqueue_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1403) 每次将新任务添加到调度程序类时都会执行。
* [dequeue_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1404) 可以从调度程序中删除任务时调用。
* [pick_next_task](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1418) 当核心调度程序代码需要确定接下来要运行的任务时，将调用此方法。
* [task_tick](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L1437) 在每个计时器滴答时调用，使调度程序类有机会测量当前任务的执行时间，并在需要抢占时通知核心调度程序代码。

有一些`sched_class`实现。通常用于所有用户任务的最常用的一种称为“完全公平调度程序（CFS）”

### 完全公平的调度程序 Completely Fair Scheduler (CFS)

CFS算法背后的原理非常简单：
1. 对于系统中的每个任务，CFS衡量已为其分配了多少CPU时间（该值存储在[sum_exec_runtime](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L385) `sched_entity`结构的字段）
1. `sum_exec_runtime` 根据任务优先级进行调整并保存在 [vruntime](https://github.com/torvalds/linux/blob/v4.14/include/linux/sched.h#L386) 区域.
1. 当`CFS`需要选择一个新任务来执行时，将选择 `vruntime` 最小的任务。

Linux调度程序使用另一个重要的数据结构，称为 `运行队列`，由 [rq](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L667) 结构. 每个CPU只有一个运行队列实例。当需要选择一个新任务来执行时，仅从本地运行队列中进行选择。但是，如果有需要，可以在不同的`rq`结构之间平衡任务。

所有调度程序类不仅使用CFS，还使用运行队列。所有CFS特定信息都保存在 [cfs_rq](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L420) 结构, 嵌入在`rq`结构中。 `cfs_rq`结构的一个重要字段称为 [min_vruntime](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L425) — 这是分配给运行队列的所有任务中最低的 `vruntime` 。将 `min_vruntime` 分配给新分叉的任务-这确保了下一步将选择该任务，因为CFS始终确保选择具有最小 `vruntime`的任务。这种方法还可以确保新任务在被抢占之前不会长时间运行。

分配给特定运行队列并由CFS跟踪的所有任务都保留在 [tasks_timeline](https://github.com/torvalds/linux/blob/v4.14/kernel/sched/sched.h#L430) `cfs_rq` 结构的字段。 `tasks_timeline` 代表一个 [Red–black tree](https://en.wikipedia.org/wiki/Red%E2%80%93black_tree), 可以用来选择按其 `vruntime` 值排序的任务。 红黑树有一个重要的属性：对它的所有操作（搜索，插入，删除）都可以在 [O(log n)](https://en.wikipedia.org/wiki/Big_O_notation) 时间. 这意味着即使系统中有成千上万的并发任务，所有调度程序方法仍将非常快速地执行。红黑树的另一个重要属性是，对于树中的任何节点，其右子节点的 `vruntime`值将始终大于父节点，而左子节点的 `vruntime` 将始终小于或等于父节点的 `vruntime`。 这有一个重要的含义：最左边的节点始终是 `vruntime` 最小的节点。

### [struct pt_regs](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/include/asm/ptrace.h#L119)

我已经向您展示了在发生中断时如何将所有通用寄存器保存在堆栈上-我们探索了该过程如何在Linux内核中工作，并为RPi OS实现了类似的过程。我还没有提到的是，在处理中断时操纵这些寄存器是完全合法的。手动准备一组寄存器并将它们放到堆栈上也是合法的-当内核线程移至用户模式时便完成了，下一课我们将实现相同的功能。现在，您只需要记住，`pt_regs`结构用于描述已保存的寄存器，并且必须将其字段的顺序与寄存器的顺序相同，并保存在`kernel_entry`宏中。在本课程中，我们将看到有关如何使用此结构的一些示例。

### 结论

与调度有关的结构，算法和概念要重要得多，但是我们刚刚探讨的是理解下两章所需的最小集合。现在是时候看看所有这些实际如何工作了。

##### 上一页

4.1 [进程调度程序：RPi OS调度程序](../../../docs/lesson04/rpi-os.md)

##### 下一页

4.3 [流程计划程序：分派任务](../../../docs/lesson04/linux/fork.md)
