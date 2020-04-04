## 5.1: 用户进程和系统调用

我们已经在RPi OS中添加了许多功能, 使其看起来像一个实际的操作系统, 而不仅仅是裸机程序.  RPi OS现在可以管理进程, 但是此功能仍然存在一个主要缺点：根本没有进程隔离. 在本课程中, 我们将解决此问题. 首先, 我们将所有用户进程移至EL0, 这将限制他们对特权处理器操作的访问. 没有此步骤, 任何其他隔离技术都没有意义, 因为任何用户程序都将能够重写我们的安全设置, 从而脱离隔离. 

如果我们限制用户程序直接访问内核功能, 这将给我们带来另一个问题. 例如, 如果用户程序需要向用户打印某些内容怎么办？我们绝对不希望它直接与UART一起使用. 相反, 如果操作系统为每个程序提供一组API方法, 那就更好了. 此类API不能实现为一组简单的方法, 因为每次用户程序要调用一种API方法时, 当前异常级别都应提高到EL1. 这种API中的各个方法称为“系统调用”, 在本课程中, 我们将向RPi OS添加一组系统调用. 

进程隔离还有第三个方面：每个进程都应该有自己独立的内存视图-我们将在第6课中解决这个问题. 

### 系统调用实现

系统调用(简称`syscalls`)背后的主要思想非常简单：每个系统调用实际上都是一个同步异常. 如果用户程序需要执行系统调用, 它首先必须准备所有必要的参数, 然后运行`svc`指令. 该指令生成同步异常. 此类异常由操作系统在EL1处理. 然后, OS验证所有参数, 执行请求的操作并执行正常的异常返回, 以确保执行将在`svc`指令后立即在EL0恢复执行.  RPi操作系统定义了4个简单的系统调用：

1. `write` 该系统调用使用UART设备在屏幕上输出某些内容. 它接受带有要打印的文本作为第一个参数的缓冲区. 
1. `clone` 该系统调用将创建一个新的用户线程. 新创建的线程的堆栈位置作为第一个参数传递. 
1. `malloc` 该系统调用为用户进程分配一个内存页. 在Linux中没有类似的系统调用(我认为在其他任何操作系统中也是如此). 我们需要它的唯一原因是RPi OS尚未实现虚拟内存, 并且所有用户进程都可以使用物理内存. 这就是每个进程都需要一种方法来确定哪个内存页面未被占用并可以使用的原因.  `malloc` 系统调用返回指向新分配页面的指针, 如果发生错误则返回`-1`. 
1. `exit` 每个进程完成执行后必须调用此`syscall`. 它将进行所有必要的清理. 

所有系统调用均在 [sys.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c) 文件中. 还有一个数组 [sys_call_table](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c) 包含指向所有`syscall`处理程序的指针.  每个系统调用都有一个 “系统调用号 `syscall number`” — 这只是 `sys_call_table` 数组. 所有系统调用号均已定义在 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/sys.h#L6) — 汇编代码使用它们来指定我们感兴趣的`syscall`. 让我们以`write` `syscall`为例, 看看`syscall`包装函数.  你可以找到它在 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L4).

```
.globl call_sys_write
call_sys_write:
    mov w8, #SYS_WRITE_NUMBER
    svc #0
    ret
```

该函数非常简单：它仅将系统调用号存储在`w8`寄存器中, 并通过执行`svc`指令生成同步异常. 按照惯例, `w8`用于系统调用编号：寄存器`x0`-`x7`用于系统调用参数, 而`x8`用于存储系统调用编号, 这允许系统调用最多包含8个参数. 

此类包装函数通常不包含在内核本身中-您更有可能在其他语言的标准库中找到它们,  如 [glibc](https://www.gnu.org/software/libc/).

### 处理同步异常

生成同步异常后,  [handler](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L98), 调用在异常表中注册的文件. 可以找到处理程序本身在 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L157) 它从以下代码开始. 

```
el0_sync:
    kernel_entry 0
    mrs    x25, esr_el1                // read the syndrome register
    lsr    x24, x25, #ESR_ELx_EC_SHIFT // exception class
    cmp    x24, #ESR_ELx_EC_SVC64      // SVC in 64-bit state
    b.eq   el0_svc
    handle_invalid_entry 0, SYNC_ERROR
```

首先, 对于所有异常处理程序, 将调用`kernel_entry`宏. 然后检查 `esr_el1`(异常综合症寄存器).  该寄存器在偏移处包含“异常类”字段 [ESR_ELx_EC_SHIFT](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/arm/sysregs.h#L46). 如果异常类等于 [ESR_ELx_EC_SVC64](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/arm/sysregs.h#L47) 这意味着当前异常是由`svc`指令引起的, 它是系统调用. 在这种情况下, 我们跳转到 `el0_svc` 标签, 否则显示错误消息. 

```
sc_nr   .req    x25                  // number of system calls
scno    .req    x26                  // syscall number
stbl    .req    x27                  // syscall table pointer

el0_svc:
    adr    stbl, sys_call_table      // load syscall table pointer
    uxtw   scno, w8                  // syscall number in w8
    mov    sc_nr, #__NR_syscalls
    bl     enable_irq
    cmp    scno, sc_nr               // check upper syscall limit
    b.hs   ni_sys

    ldr    x16, [stbl, scno, lsl #3] // address in the syscall table
    blr    x16                       // call sys_* routine
    b      ret_from_syscall
ni_sys:
    handle_invalid_entry 0, SYSCALL_ERROR
```

首先, `el0_svc`将系统调用表的地址加载到`stbl`中(它只是`x27`寄存器的别名. ), 将系统调用号加载到`scno`变量中. 然后启用中断, 并将系统调用数与系统中系统调用的总数进行比较-如果大于或等于, 则会显示一条错误消息. 如果`syscall`编号在要求的范围内, 它将用作`syscall`表数组中的索引, 以获得指向`syscall`处理程序的指针. 接下来, 执行处理程序, 并在完成后调用`ret_from_syscall`. 注意, 这里我们不要触摸寄存器 `x0` – `x7` – 它们被透明地传递给处理程序. 

```
ret_from_syscall:
    bl    disable_irq
    str   x0, [sp, #S_X0]             // returned x0
    kernel_exit 0
```

`ret_from_syscall`首先禁用中断. 然后将x0寄存器的值保存在堆栈中. 这是必需的, 因为`kernel_exit`将从其保存的值中恢复所有通用寄存器, 但是 `x0` 现在包含`syscall` 处理程序的返回值, 我们希望将此值传递给用户代码. 最后, `kernel_exit`被调用, 返回到用户代码. 

### 在EL0和EL1之间切换

如果您仔细阅读了以前的课程, 您可能会注意到`kernel_entry`和`kernel_exit`宏中的变化：现在它们都接受一个附加参数. 此参数指示从哪个异常级别获取异常. 正确保存/恢复堆栈指针需要有关原始异常级别的信息. 这是来自`kernel_entry`和`kernel_exit`宏的两个相关部分. 

```
    .if    \el == 0
    mrs    x21, sp_el0
    .else
    add    x21, sp, #S_FRAME_SIZE
    .endif /* \el == 0 */
```

```
    .if    \el == 0
    msr    sp_el0, x21
    .endif /* \el == 0 */
```

我们为`EL0`和`EL1`使用了2个不同的堆栈指针, 这就是为什么从`EL0`提取异常后堆栈指针会被覆盖的原因. 原始堆栈指针可在 `sp_el0` 寄存器中找到. 即使在异常处理程序中不触摸`sp_el0`, 也必须在发生异常之前和之后存储并恢复该寄存器的值. 如果不这样做, 在上下文切换之后, 最终将在`sp`寄存器中得到错误的值. 

您还可能会问, 如果从EL1中获取异常, 为什么我们不恢复`sp`寄存器的值呢？那是因为我们正在为异常处理程序重用相同的内核堆栈. 即使在异常处理过程中发生上下文切换, 在`kernel_exit`时, `sp`也已被`cpu_switch_to`函数切换.  (顺便说一句, 在Linux中, 行为是不同的, 因为`Linux`使用不同的堆栈作为中断处理程序. )

还值得注意的是, 我们不需要在`eret`指令之前明确指定需要返回的异常级别. 这是因为此信息是在`spsr_el1`寄存器中编码的, 因此我们总是返回到发生异常的级别. 

### 将任务移至用户模式

在进行任何系统调用之前, 我们显然需要有一个在用户模式下运行的任务. 有两种创建新用户任务的可能性：将内核线程移至用户模式, 或者用户任务可以派生自身来创建新的用户任务. 在本节中, 我们将探讨第一种可能性. 

实际完成工作的功能称为 [move_to_user_mode](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/fork.c#Li47), 但是在研究它之前, 让我们先检查一下如何使用此函数. 为此, 您需要先打开 [kernel.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c)  文件. 让我在这里复制相关行. 

```cpp
    int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, 0);
    if (res < 0) {
        printf("error while starting kernel process");
        return;
    }
```

首先, 在`kernel_main`函数中, 我们创建一个新的内核线程. 我们这样做的方式与上一课相同. 调度程序运行新创建的任务后, 将在内核模式下执行`kernel_process`函数. 

```cpp
void kernel_process(){
    printf("Kernel process started. EL %d\r\n", get_el());
    int err = move_to_user_mode((unsigned long)&user_process);
    if (err < 0){
        printf("Error while moving process to user mode\n\r");
    }
}
```

然后, `kernel_process`打印状态消息并调用`move_to_user_mode`, 将指向`user_process`的指针作为第一个参数. 现在, 让我们看看`move_to_user_mode`函数正在做什么. 

```cpp
int move_to_user_mode(unsigned long pc)
{
    struct pt_regs *regs = task_pt_regs(current);
    memzero((unsigned long)regs, sizeof(*regs));
    regs->pc = pc;
    regs->pstate = PSR_MODE_EL0t;
    unsigned long stack = get_free_page(); //allocate new user stack
    if (!stack) {
        return -1;
    }
    regs->sp = stack + PAGE_SIZE;
    current->stack = stack;
    return 0;
}
```

现在, 我们处于执行从init任务派生的内核线程的过程中. 在上一课中, 我们讨论了分叉过程, 并且看到在新创建的任务堆栈的顶部保留了一个小区域( `pt_regs`区域). 这是我们第一次使用此区域：我们将在此处保存手动准备的处理器状态.  该状态将具有与`kernel_exit`宏期望的格式完全相同的格式, 并且其结构由 [pt_regs](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/fork.h#L21) 结构组成.

`pt_regs`结构的以下字段在`move_to_user_mode`函数中初始化. 

* `pc` 现在, 它指向需要在用户模式下执行的功能.  `kernel_exit`将把`pc`复制到`elr_el1`寄存器中, 从而确保在执行异常返回后我们将返回到`pc`地址. 
* `pstate` 该字段将由`kernel_exit`复制到`spsr_el1`, 并在异常返回完成后成为处理器状态.  [PSR_MODE_EL0t](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/include/fork.h#L9) 常量, 复制到 `pstate` 字段中的代码的方式是将异常返回到`EL0`级别. 从`EL3`切换到`EL1`时, 我们已经在第2课中做了相同的技巧. 
* `stack` `move_to_user_mode`  为用户堆栈分配一个新页面, 并将 `sp` 字段设置为指向该页面的顶部. 

`task_pt_regs` 函数用于计算 `pt_regs` 区域的位置. 由于我们初始化当前内核线程的方式, 我们确保在完成后, `sp`会指向`pt_regs`区域之前. 这发生在 [ret_from_fork](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/entry.S#L188) 函数中.

```
.globl ret_from_fork
ret_from_fork:
    bl    schedule_tail
    cbz   x19, ret_to_user            // not a kernel thread
    mov   x0, x20
    blr   x19
ret_to_user:
    bl disable_irq
    kernel_exit 0
```

您可能会注意到`ret_from_fork`已更新. 现在, 在内核线程完成之后, 执行转到 `ret_to_user` 标签, 此处我们使用先前准备的处理器状态来禁用中断并执行正常的异常返回. 

### Forking 用户进程

现在让我们回到`kernel.c`文件. 如上一节所述, `kernel_process`完成之后, [user_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c#L22) 函数将在用户模式下执行. 该函数调用`clone`系统调用两次以执行 [user_process1](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/kernel.c#L10) 在2个并行线程中起作用.  `clone`系统调用要求将新的用户堆栈的位置传递给它, 我们还需要调用`malloc` syscall以便分配2个新的内存页. 现在让我们看一下`clone` syscall包装函数的外观.  你可以在 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.S#L22) 找到它.

```
.globl call_sys_clone
call_sys_clone:
    /* Save args for the child.  */
    mov    x10, x0                    /*fn*/
    mov    x11, x1                    /*arg*/
    mov    x12, x2                    /*stack*/

    /* Do the system call.  */
    mov    x0, x2                     /* stack  */
    mov    x8, #SYS_CLONE_NUMBER
    svc    0x0

    cmp    x0, #0
    beq    thread_start
    ret

thread_start:
    mov    x29, 0

    /* Pick the function arg and execute.  */
    mov    x0, x11
    blr    x10

    /* We are done, pass the return value through x0.  */
    mov    x8, #SYS_EXIT_NUMBER
    svc    0x0
```

在 `clone` 系统调用包装函数的设计中, 我试图模拟 [coresponding function](https://sourceware.org/git/?p=glibc.git;a=blob;f=sysdeps/unix/sysv/linux/aarch64/clone.S;h=e0653048259dd9a3d3eb3103ec2ae86acb43ef48;hb=HEAD#l35) 从`glibc`库. 此功能执行以下操作. 

1. 保存寄存器 `x0`-`x3`, 这些寄存器包含`syscall`的参数, 稍后将被`syscall`处理程序覆盖. 
1. 调用`syscall`处理程序. 
1. 检查`syscall`处理程序的返回值：如果它是`0`, 我们正在新创建的线程内执行. 在这种情况下, 执行将转到 `thread_start` 标签. 
1. 如果返回值不为零, 则它是新任务的`PID`. 这意味着, 我们的系统调用完成后回到这里, 在我们原来的线程内部执行 - 只返回在这种情况下的调用者. 
1. 最初作为第一个参数传递的函数在新线程中调用. 
1. 函数完成后, 将执行`exit`系统调用 - 它永远不会返回. 

如您所见, 克隆包装函数和克隆syscall的语义不同：前者接受指向要执行的函数的指针作为参数, 而后者则两次返回调用者：第一次是在原始任务中, 第二次是在克隆的任务. 

可以找到克隆的系统调用处理程序在 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sys.c#L11). 这非常简单, 它只调用已经熟悉的`copy_process`函数. 但是, 此功能自上一课以来已被修改-现在它支持克隆用户线程和内核线程. 下面列出了该函数的来源. 

```cpp
int copy_process(unsigned long clone_flags, unsigned long fn, unsigned long arg, unsigned long stack)
{
    preempt_disable();
    struct task_struct *p;

    p = (struct task_struct *) get_free_page();
    if (!p) {
        return -1;
    }

    struct pt_regs *childregs = task_pt_regs(p);
    memzero((unsigned long)childregs, sizeof(struct pt_regs));
    memzero((unsigned long)&p->cpu_context, sizeof(struct cpu_context));

    if (clone_flags & PF_KTHREAD) {
        p->cpu_context.x19 = fn;
        p->cpu_context.x20 = arg;
    } else {
        struct pt_regs * cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE;
        p->stack = stack;
    }
    p->flags = clone_flags;
    p->priority = current->priority;
    p->state = TASK_RUNNING;
    p->counter = p->priority;
    p->preempt_count = 1; //disable preemtion until schedule_tail

    p->cpu_context.pc = (unsigned long)ret_from_fork;
    p->cpu_context.sp = (unsigned long)childregs;
    int pid = nr_tasks++;
    task[pid] = p;
    preempt_enable();
    return pid;
}
```

以防万一, 当我们创建一个新的内核线程时, 该函数的行为完全相同, 如上一课中所述. 在另一种情况下, 当我们克隆用户线程时, 将执行这部分代码. 

```cpp
        struct pt_regs * cur_regs = task_pt_regs(current);
        *childregs = *cur_regs;
        childregs->regs[0] = 0;
        childregs->sp = stack + PAGE_SIZE;
        p->stack = stack;
```

我们在这里要做的第一件事是访问处理器状态, 该状态由 `kernel_entry` 宏保存. 但是, 为什么我们可以使用相同的 `task_pt_regs` 函数, 该函数仅返回内核堆栈顶部的 `pt_regs` 区域, 这并不明显. 为什么 `pt_regs` 不可能存储在堆栈中的其他位置？答案是只有在调用`clone` syscall后才能执行此代码. 在触发 `syscall` 时, 当前的内核堆栈为空(进入用户模式后我们将其保留为空). 这就是为什么 `pt_regs` 将始终存储在内核堆栈的顶部. 对于所有后续的系统调用, 将保留该规则, 因为它们中的每一个在返回用户模式之前都会使内核堆栈为空. 

在第二行中, 当前处理器状态被复制到新任务的状态. 新状态下的`x0`设置为`0`, 因为调用者会将`x0`解释为`syscall`的返回值. 我们已经看到克隆包装器函数如何使用此值来确定我们是否仍在执行原始线程的一部分还是新线程. 

新任务的下一个 `sp` 设置为指向新用户堆栈页面的顶部. 我们还将指针保存到堆栈页面, 以便在任务完成后进行清理. 

### 退出任务

每个用户任务完成后, 应调用 `exit` 系统调用(在当前实现中, `clone` 包装器函数会隐式调用 `exit`).   `exit` `syscall`然后调用 [exit_process](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson05/src/sched.c) 函数, 负责停用任务. 该功能在下面列出. 

```cpp
void exit_process(){
    preempt_disable();
    for (int i = 0; i < NR_TASKS; i++){
        if (task[i] == current) {
            task[i]->state = TASK_ZOMBIE;
            break;
        }
    }
    if (current->stack) {
        free_page(current->stack);
    }
    preempt_enable();
    schedule();
}
```

按照Linux约定, 我们不会立即删除任务, 而是将其状态设置为`TASK_ZOMBIE`. 这样可以防止调度程序选择和执行任务. 在Linux中, 这种方法用于允许父进程即使在子进程完成后也可以查询有关该子进程的信息. 

`exit_process`现在也删除了不必要的用户堆栈并调用`schedule`. 在调用`计划`后, 将选择新任务, 这就是该系统调用永不返回的原因. 

### 结论

既然RPi OS可以管理用户任务, 那么我们将更接近于完全的流程隔离. 但是仍然缺少一个重要步骤：所有用户任务共享相同的物理内存, 并且可以轻松读取彼此的数据. 在下一课中, 我们将介绍虚拟内存并解决此问题. 

##### 上一页

4.5 [进程计划程序：练习](../../docs/lesson04/exercises.md)

##### 下一页

5.2 [用户进程和系统调用：Linux](../../docs/lesson05/linux.md)
