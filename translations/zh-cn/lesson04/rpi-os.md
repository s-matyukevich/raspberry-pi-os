## 4.1: Scheduler

目前为止， PRi OS已经是一个相当复杂的裸机程序，但是说实话，我们仍然不能将其称为操作系统。原因是它无法完成任何OS应该执行的任何核心任务。这种核心任务之一称为流程调度。通过调度，我的意思是操作系统应该能够在不同进程之间共享CPU时间。其中最困难的部分是，一个进程应该不知道调度的发生：它应该将自己视为唯一占用CPU的进程。在本课程中，我们将将此功能添加到RPi OS。


### task_struct

如果要管理流程，我们应该做的第一件事就是创建一个描述流程的结构。 Linux具有这样的结构，它称为`task_struct`（在Linux中，线程和进程只是不同类型的任务）。 由于我们主要模仿Linux的实现，因此我们将做同样的事情。 RPi OS [task_struct](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L36) 如下所示。

```cpp
struct cpu_context {
    unsigned long x19;
    unsigned long x20;
    unsigned long x21;
    unsigned long x22;
    unsigned long x23;
    unsigned long x24;
    unsigned long x25;
    unsigned long x26;
    unsigned long x27;
    unsigned long x28;
    unsigned long fp;
    unsigned long sp;
    unsigned long pc;
};

struct task_struct {
    struct cpu_context cpu_context;
    long state;
    long counter;
    long priority;
    long preempt_count;
};
```

该结构具有以下成员：

* `cpu_context` 这是一个嵌入式结构，其中包含正在切换的任务之间可能不同的所有寄存器的值。 有一个合理的问题是为什么我们不保存所有寄存器，而只保存寄存器 `x19-x30` 和 `sp` ？(`fp` 是 `x29` 并且 `pc` 是 `x30`) 答案是实际的上下文切换仅在任务中调用[cpu_switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.S#L4)函数. 因此，从正在切换的任务的角度来看，它仅调用`cpu_switch_to`函数，并在一段时间（可能很长）后返回。该任务不会注意到在此期间发生了另一个任务。根据ARM的调用约定，寄存器 `x0-x18` 可以被调用的函数覆盖，因此调用者不得假定这些寄存器的值在函数调用后仍然存在。这就是为什么保存`x0-x18`寄存器没有意义的原因。
* `state` 这是当前正在运行的任务的状态。对于仅在CPU上做一些工作的任务，状态始终为 [TASK_RUNNING](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L15). 实际上，这是RPi OS目前要支持的唯一状态。但是，稍后我们将不得不添加一些其他状态。例如，等待中断的任务应移至其他状态，因为在尚未发生所需的中断时唤醒任务是没有意义的。
* `counter` 该字段用于确定当前任务已运行多长时间。 `计数器`会在每个计时器滴答时减少1，到0时便会安排另一个任务。
* `priority`  安排新任务时，将其 `优先级` 复制到 `计数器` 中。通过设置任务优先级，我们可以调节任务相对于其他任务获得的处理器时间。
* `preempt_count` 如果该字段的值为非零值，则表明当前任务正在执行一些不可中断的关键功能（例如，它运行调度功能）。如果在此时间发生计时器滴答，则将忽略它，并且不会触发重新计划。

内核启动后，只有一个任务正在运行：一个正在运行 [kernel_main](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L19) 函数. 它称为“初始化任务”。在启用调度程序功能之前，我们必须填充与初始化任务相对应的 `task_struct`。 这个被完成在 [这](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/sched.h#L53).

所有任务都存储在 [task](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L7) 数组. 该阵列只有64个插槽-这是我们在RPi OS中可以同时执行的最大任务数。对于生产就绪的OS来说，它绝对不是最佳解决方案，但对于我们的目标而言，这是可以的。

还有一个非常重要的变量称为 [current](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L6) 总是指向当前正在执行的任务。 `current` 和 `task` 数组都初始设置为持有指向init任务的指针。 还有一个全局变量称为 [nr_tasks](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L8) - 它包含系统中当前正在运行的任务数。

这些都是我们将用于实现调度程序功能的结构和全局变量。在对`task_struct`的描述中，我已经简要提到了调度工作的一些方面，因为如果不了解如何使用特定的`task_struct`字段，就无法理解其含义。现在我们将更详细地研究调度算法，我们将从 `kernel_main` 功能开始。

### `kernel_main` 函数

在深入探讨调度程序实现之前，我想快速向您展示如何证明调度程序确实有效。 要了解它，您需要看一下 [kernel.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c) 文件. 让我在此处复制相关内容。

```cpp
void kernel_main(void)
{
    uart_init();
    init_printf(0, putc);
    irq_vector_init();
    timer_init();
    enable_interrupt_controller();
    enable_irq();

    int res = copy_process((unsigned long)&process, (unsigned long)"12345");
    if (res != 0) {
        printf("error while starting process 1");
        return;
    }
    res = copy_process((unsigned long)&process, (unsigned long)"abcde");
    if (res != 0) {
        printf("error while starting process 2");
        return;
    }

    while (1){
        schedule();
    }
}
```

关于此代码，有一些重要的事情。

1. 新函数 [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) 介绍。 `copy_process` 需要2个参数: 在新线程中执行的函数以及需要传递给该函数的参数。 `copy_process` 分配一个新的 `task_struct`  并使其可用于调度程序。
2. 我们的另一个新函数称为 [schedule](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L21). 这是核心调度程序功能：它检查是否有新任务需要抢占当前任务。如果任务目前没有任何工作，可以自动调用`计划`。 计时器中断处理程序也会调用 `schedule`。

我们两次调用`copy_process`， 每次传递指向 [process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L9) 作为第一个参数。处理功能非常简单。

```cpp
void process(char *array)
{
    while (1){
        for (int i = 0; i < 5; i++){
            uart_send(array[i]);
            delay(100000);
        }
    }
}
```

它只是一直在屏幕上打印数组中的字符，将其作为参数传递。第一次使用参数 `12345` 调用它，第二次使用 `abcde` 参数。如果我们的调度程序实现正确，我们应该在屏幕上看到两个线程的混合输出。

### 内存分配

系统中的每个任务都应具有其专用的堆栈。这就是为什么在创建新任务时我们必须有一种分配内存的方法。目前，我们的内存分配器非常原始。 (可以在以下位置找到实现 [mm.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/mm.c) 文件)

```cpp
static unsigned short mem_map [ PAGING_PAGES ] = {0,};

unsigned long get_free_page()
{
    for (int i = 0; i < PAGING_PAGES; i++){
        if (mem_map[i] == 0){
            mem_map[i] = 1;
            return LOW_MEMORY + i*PAGE_SIZE;
        }
    }
    return 0;
}

void free_page(unsigned long p){
    mem_map[p / PAGE_SIZE] = 0;
}
```

分配器只能与内存页面一起使用（每个页面的大小为4 KB）。有一个名为 `mem_map` 的数组，该数组对于系统中的每个页面都保持其状态：分配还是空闲。 每当我们需要分配一个新页面时，我们就循环遍历此数组并返回第一个空闲页面。此实现基于两个假设：

1. 我们知道系统中的内存总量。 它是 `1 GB - 1 MB` (存储器的最后兆字节为设备寄存器保留。). 此值存储在 [HIGH_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L14) 常量中.
1. 前4 MB的内存保留给内核映像和init任务堆栈。 此值存储在 [LOW_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L13) 常量. 所有内存分配都在此之后开始。

### 创建一个新任务

新任务分配在 [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) 函数.

```cpp
int copy_process(unsigned long fn, unsigned long arg)
{
    preempt_disable();
    struct task_struct *p;

    p = (struct task_struct *) get_free_page();
    if (!p)
        return 1;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.x19 = fn;
    p->cpu_context.x20 = arg;
    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
    int pid = nr_tasks++;
    task[pid] = p;
    preempt_enable();
    return 0;
}
```

现在，我们将详细研究它。

```
    preempt_disable();
    struct task_struct *p;
```

该函数从禁用抢占和为新任务分配指针开始。抢占已禁用，因为我们不想在 `copy_process` 函数中间将其重新安排到其他任务。

```
    p = (struct task_struct *) get_free_page();
    if (!p)
        return 1;
```

接下来，分配一个新页面。在此页面的底部，我们为新创建的任务放置 `task_struct`。该页面的其余部分将用作任务堆栈。

```cpp
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail
```

分配好`task_struct`之后，我们可以初始化其属性。优先级和初始计数器是根据当前任务优先级设置的。状态设置为 `TASK_RUNNING` ，表示新任务已准备好开始。 `preempt_count`设置为1，这意味着在执行任务之后，在完成一些初始化工作之前，不应重新计划其时间。

```cpp
    p->cpu_context.x19 = fn;
    p->cpu_context.x20 = arg;
    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)p + THREAD_SIZE;
```

这是功能最重要的部分。 这里 `cpu_context` 被初始化. 堆栈指针设置在新分配的内存页面的顶部。 `pc`  被设置为 [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L146) 函数, 并且我们现在需要看一下这个函数，以便理解为什么其余`cpu_context`寄存器以它们的方式初始化。

```cpp
.globl ret_from_fork
ret_from_fork:
    bl    schedule_tail
    mov    x0, x20
    blr    x19         //should never return
```

如您所见，`ret_from_fork`第一次调用 [schedule_tail](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L65), 只是启用了抢占，然后使用存储在`x20`中的参数调用存储在`x19`寄存器中的函数。 在调用`ret_from_fork`函数之前，从`cpu_context`中恢复出`x19`和`x20`。

现在，让我们回到`copy_process`。

```
    int pid = nr_tasks++;
    task[pid] = p;
    preempt_enable();
    return 0;
```

最后，`copy_process` 将新创建的任务添加到 `task` 数组中，并为当前任务启用抢占。

关于`copy_process`函数要了解的重要一点是，它在完成执行后不会发生上下文切换。该函数仅准备新的`task_struct`并将其添加到`task`数组中-仅在调用`schedule`函数后才执行此任务。

### 谁调用 `schedule`?

在深入了解`schedule`功能之前，首先要弄清楚`schedule`的调用方式。有2种情况。

1. 当一个任务现在没有任何事情要做，但是仍然无法终止时，它可以自动调用`schedule`。那就是`kernel_main`函数所做的。
1. `schedule` 也定期从 [timer interrupt handler 时钟终端句柄](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/timer.c#L21).

现在让我们来看看 [timer_tick](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L70) 函数, 从计时器中断中调用。

```cpp
void timer_tick()
{
    --current->counter;
    if (current->counter>0 || current->preempt_count >0) {
        return;
    }
    current->counter=0;
    enable_irq();
    _schedule();
    disable_irq();
```

首先，它减少了当前任务的计数器。如果计数器大于0，或者当前禁用了抢占功能，则返回该函数，否则调用`schedule`并启用中断。 （我们在中断处理程序内部，默认情况下禁用中断）。在下一部分中，我们将了解为什么在调度程序执行期间必须启用中断。

### Scheduling algorithm

最后，我们准备看一下调度程序算法。我几乎是从Linux内核的第一个发行版中精确复制了此算法。您可以找到原始版本在 [这里](https://github.com/zavg/linux-0.01/blob/master/kernel/sched.c#L68).

```cpp
void _schedule(void)
{
    preempt_disable();
    int next,c;
    struct task_struct * p;
    while (1) {
        c = -1;
        next = 0;
        for (int i = 0; i < NR_TASKS; i++){
            p = task[i];
            if (p && p->state == TASK_RUNNING && p->counter > c) {
                c = p->counter;
                next = i;
            }
        }
        if (c) {
            break;
        }
        for (int i = 0; i < NR_TASKS; i++) {
            p = task[i];
            if (p) {
                p->counter = (p->counter >> 1) + p->priority;
            }
        }
    }
    switch_to(task[next]);
    preempt_enable();
}
```

该算法的工作原理如下：

 * 第一个内部的`for`循环遍历所有任务，并尝试以最大计数器找到处于`TASK_RUNNING`状态的任务。如果找到了这样的任务，并且其计数器大于0，我们立即从外部的 `while` 循环中中断，并切换到该任务。如果找不到这样的任务，则意味着当前不存在处于 `TASK_RUNNING` 状态的任务，或者所有此类任务的计数器均为0。在实际的OS中，例如，当所有任务都在等待中断时，可能会发生第一种情况。在这种情况下，将执行第二个嵌套的 `for` 循环。对于每个任务（无论处于什么状态），此循环都会增加其计数器。计数器增加以非常聪明的方式完成：


    1. 任务通过的第二个`for` 循环的迭代次数越多，其计数器的计数就越高。
    2. 任务计数器永远不能超过 `2 *优先级`。

* 然后重复该过程。如果至少有一个任务处于`TASK_RUNNIG`状态，则外部`while`循环的第二次迭代将是最后一个，因为在第一次迭代之后，所有计数器都已经非零。但是，如果没有 `TASK_RUNNING` 任务，则该过程会反复进行，直到某些任务变为 `TASK_RUNNING` 状态。但是，如果我们在单个CPU上运行，那么在此循环运行时如何更改任务状态？答案是，如果某些任务正在等待中断，则该中断可能在执行`计划`功能时发生，并且中断处理程序可以更改任务的状态。这实际上解释了为什么在“计划”执行期间必须启用中断。这也说明了禁用中断和禁用抢占之间的重要区别。时间表会在整个功能期间禁用抢占。这样可以确保在我们执行原始函数的过程中不会调用嵌套的 `schedule`。但是，在 `计划` 函数执行期间，中断可能合法发生。

我非常注意某些任务正在等待中断的情况，尽管RPi OS尚未实现此功能。但是我仍然认为有必要了解这种情况，因为它是核心调度程序算法的一部分，以后将添加类似的功能。

### 切换任务

找到具有非零计数器的 `TASK_RUNNING` 状态的任务后, [switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L56) 函数被调用。 看起来像这样。

```cpp
void switch_to(struct task_struct * next)
{
    if (current == next)
        return;
    struct task_struct * prev = current;
    current = next;
    cpu_switch_to(prev, next);
}
```

在这里，我们检查下一个过程是否与当前过程不同，如果不一致，则更新`current`变量。实际工作被重定向到 [cpu_switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.S) 函数.

```cpp
.globl cpu_switch_to
cpu_switch_to:
    mov    x10, #THREAD_CPU_CONTEXT
    add    x8, x0, x10
    mov    x9, sp
    stp    x19, x20, [x8], #16        // store callee-saved registers
    stp    x21, x22, [x8], #16
    stp    x23, x24, [x8], #16
    stp    x25, x26, [x8], #16
    stp    x27, x28, [x8], #16
    stp    x29, x9, [x8], #16
    str    x30, [x8]
    add    x8, x1, x10
    ldp    x19, x20, [x8], #16        // restore callee-saved registers
    ldp    x21, x22, [x8], #16
    ldp    x23, x24, [x8], #16
    ldp    x25, x26, [x8], #16
    ldp    x27, x28, [x8], #16
    ldp    x29, x9, [x8], #16
    ldr    x30, [x8]
    mov    sp, x9
    ret
```

这是实际上下文切换发生的地方。让我们逐行检查它。

```
    mov    x10, #THREAD_CPU_CONTEXT
    add    x8, x0, x10
```

`THREAD_CPU_CONTEXT`常量包含`task_struct`中的`cpu_context`结构的偏移量。 `x0`包含一个指向第一个参数的指针，该指针是当前的`task_struct`（在这里，当前是指我们要从中切换的那个）。复制的两行执行后，`x8`将包含指向当前`cpu_context`的指针。

```
    mov    x9, sp
    stp    x19, x20, [x8], #16        // store callee-saved registers
    stp    x21, x22, [x8], #16
    stp    x23, x24, [x8], #16
    stp    x25, x26, [x8], #16
    stp    x27, x28, [x8], #16
    stp    x29, x9, [x8], #16
    str    x30, [x8]
```

接下来，所有保存有`calle`的寄存器都按照在`cpu_context`结构中定义的顺序存储。 `x30`是链接寄存器，包含函数返回地址，存储为`pc`，当前堆栈指针存储为`sp`，`x29`存储为`fp`（帧指针）。

```
    add    x8, x1, x10
```

现在，`x10`包含在`task_struct`内部的`cpu_context`结构的偏移量，`x1`是指向下一个`task_struct`的指针，因此`x8`将包含指向下一个`cpu_context`的指针。

```
    ldp    x19, x20, [x8], #16        // restore callee-saved registers
    ldp    x21, x22, [x8], #16
    ldp    x23, x24, [x8], #16
    ldp    x25, x26, [x8], #16
    ldp    x27, x28, [x8], #16
    ldp    x29, x9, [x8], #16
    ldr    x30, [x8]
    mov    sp, x9
```

被调用者保存的寄存器从下一个`cpu_context`恢复。

```
    ret
```

函数返回到链接寄存器（`x30`）所指向的位置。如果我们是第一次切换到某个任务，则将 [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L148) 函数。 在所有其他情况下，该位置将是先前由`cpu_switch_to`函数保存在`cpu_context`中的位置。

### 调度如何与异常进入/退出一起工作？

在上一课中，我们看到了[kernel_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L17) 和 [kernel_exit](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L4) 宏用于保存和恢复处理器状态。 引入调度程序后，出现了一个新问题：现在完全可以合法地将中断作为一项任务输入，而将其保留为另一项任务。这是一个问题，因为我们用来从中断返回的 `eret` 指令依赖于以下事实：返回地址应存储在 `elr_el1` 中，处理器状态应存储在 `spsr_el1` 寄存器中。因此，如果要在处理中断时切换任务，则必须将这两个寄存器与所有其他通用寄存器一起保存和恢复。 这样做的代码非常简单，您可以找到保存部分在 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L35) 和 恢复到 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L46).

### 在上下文切换期间跟踪系统状态

我们已经检查了与上下文切换有关的所有源代码。但是，该代码包含许多异步交互，这使得很难完全了解整个系统的状态如何随时间变化。在本节中，我想让您更轻松地完成此任务：我想描述从系统启动到第二次上下文切换之时发生的事件的顺序。对于每个此类事件，我还将包括一个表示事件发生时存储器状态的图表。我希望这种表示形式将帮助您深入了解调度程序的工作方式。所以，让我们开始吧！

1. 内核已初始化并 [kernel_main](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L19) 函数已被执行. 初始堆栈配置为开始于 [LOW_MEMORY](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/include/mm.h#L13), 这是4 MB。
    
    ```
             0 +------------------+
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```

1. `kernel_main` 首次调用 [copy_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/fork.c#L5) 。 分配了新的4 KB内存页面，并在该页面的底部放置了 `task_struct`。 （稍后，我将在此时创建的任务称为`任务1`）
    
    ```
             0 +------------------+
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               | task_struct 1    |
               |------------------|
               |                  |
    0x00401000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```
    
1.  `kernel_main`第二次调用`copy_process`并且重复相同的过程。任务2已创建并添加到任务列表。
    
    ```
             0 +------------------+
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               | task_struct 1    |
               |------------------|
               |                  |
    0x00401000 +------------------+
               | task_struct 2    |
               |------------------|
               |                  |
    0x00402000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```

1. `kernel_main` 自动调用 [schedule](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L21) 功能并决定运行任务1。
1. [cpu_switch_to](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.S#L4) 将保存了`calee`的寄存器保存在位于内核映像内部的init任务 `cpu_context` 中。
1. `cpu_switch_to` 从任务1恢复已保存日历的寄存器 `cpu_context`. `sp` 现在指向 `0x00401000`, 链接注册到 [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L146) 函数, `x19` 包含一个指向 [process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/kernel.c#L9) 函数 和 `x20` 一个指向字符串 `12345`的指针，该字符串位于内核映像中的某个位置。
1. `cpu_switch_to` 调用`ret`指令，该指令跳转到`ret_from_fork`函数。
1. `ret_from_fork` 读取 `x19` 和 `x20` 寄存器，并使用参数 `12345` 调用 `process` 函数。在`process`函数开始执行后，其堆栈开始增长。
    
    ```
             0 +------------------+
               | kernel image     |
               |------------------|
               |                  |
               |------------------|
               | init task stack  |
    0x00400000 +------------------+
               | task_struct 1    |
               |------------------|
               |                  |
               |------------------|
               | task 1 stack     |
    0x00401000 +------------------+
               | task_struct 2    |
               |------------------|
               |                  |
    0x00402000 +------------------+
               |                  |
               |                  |
    0x3F000000 +------------------+
               | device registers |
    0x40000000 +------------------+
    ```

1. 发生计时器中断。 [kernel_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/entry.S#L17) 宏保存所有通用寄存器 + `elr_el1` 和 `spsr_el1` 到任务1堆栈的底部。

    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```

1. `schedule` 被调用 并且它决定运行任务2。但是我们仍然运行任务1，并且其堆栈继续增长到任务1保存的寄存器区域以下。在图中，堆栈的这一部分标记为（int），表示“中断堆栈”

    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack (int)     |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```

1. `cpu_switch_to` 运行任务2。为此，它执行与任务1完全相同的步骤序列。任务2开始执行，并且堆栈不断增长。请注意，我们此时并未从中断返回， 但这没关系，因为现在已启用中断 (先前已在 [timer_tick](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L70) 之前 `schedule` 被调用)
    
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack (int)     |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```

1. 另一个定时器中断发生，`kernel_entry`将所有通用寄存器+`elr_el1`和`spsr_el1`保存在任务2堆栈的底部。任务2中断堆栈开始增长。

    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack (int)     |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack (int)     |
               |------------------------|
               | task 2 saved registers |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```

1. `schedule` 被调用. 它观察到所有任务的计数器都设置为0，并将计数器设置为任务优先级。
1. `schedule` 选择要运行的初始化任务. （这是因为现在所有任务的计数器都设置为1，而`init`任务是列表中的第一个）。但是实际上，此时 `计划` 选择任务1或任务2是完全合法的，因为它们的计数器值相等。我们对选择任务1的情况更感兴趣，所以现在让我们假设这是发生了什么。
1. `cpu_switch_to` 被调用 并从任务1恢复以前保存的被调用方保存的寄存器 `cpu_context`. 链接现在注册要点到 [这](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L63) 因为这是上次执行任务1时调用`cpu_switch_to`的位置。 `sp` 指向任务1中断堆栈的底部。
1. `timer_tick` 函数恢复执行, 从 [这](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L79) 行开始. 最终禁用中断 [kernel_exit](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson04/src/sched.c#L79) 被执行。 到调用 `kernel_exit` 时，任务1的中断堆栈已折叠为0。
    
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 saved registers |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack (int)     |
               |------------------------|
               | task 2 saved registers |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```

1. `kernel_exit` 恢复所有通用寄存器以及`elr_el1` 和 `spsr_el1`. `elr_el1` 现在指向 `process` 函数中间的某个位置。 `sp`指向任务1堆栈的底部。
    
    ```
             0 +------------------------+
               | kernel image           |
               |------------------------|
               |                        |
               |------------------------|
               | init task stack        |
    0x00400000 +------------------------+
               | task_struct 1          |
               |------------------------|
               |                        |
               |------------------------|
               | task 1 stack           |
    0x00401000 +------------------------+
               | task_struct 2          |
               |------------------------|
               |                        |
               |------------------------|
               | task 2 stack (int)     |
               |------------------------|
               | task 2 saved registers |
               |------------------------|
               | task 2 stack           |
    0x00402000 +------------------------+
               |                        |
               |                        |
    0x3F000000 +------------------------+
               | device registers       |
    0x40000000 +------------------------+
    ```

1. `kernel_exit` 执行 `eret` 使用的指令 `elr_el1` 注册以跳转回 `process` 函数. 任务1恢复其正常执行。

上述步骤顺序非常重要 - 我个人认为这是整个教程中最重要的事情之一。如果您在理解上有困难，我可以建议您从本课开始进行练习1。

### 结论

我们已经完成了调度，但是现在我们的内核只能管理内核线程：它们在`EL1`上执行，并且可以直接访问任何内核函数或数据。在接下来的2节课中，我们将解决此问题，并介绍系统调用和虚拟内存。

##### 上一页

3.5 [中断处理：练习](../../docs/lesson03/exercises.md)

##### 下一页

4.2 [流程调度程序：调度程序的基本结构](../../docs/lesson04/linux/basic_structures.md)
