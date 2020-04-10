## 3.1: 中断

从第1课开始, 我们已经知道如何与硬件通信. 但是, 大多数情况下, 沟通方式并不是那么简单. 通常, 这种模式是异步的：我们向设备发送一些命令, 但它不会立即响应. 而是在工作完成时通知我们. 此类异步通知称为“中断”, 因为它们会中断正常的执行流并迫使处理器执行“中断处理程序”. 

有一种设备在操作系统开发中特别有用：系统计时器. 它是一种可以配置为以某个预定频率定期中断处理器的设备. 在过程调度中使用计时器的一种特殊应用. 调度程序需要测量每个进程执行了多长时间, 并使用此信息选择要运行的下一个进程. 此测量基于计时器中断. 

在下一课中, 我们将详细讨论进程调度, 但是现在, 我们的任务是初始化系统计时器并实现计时器中断处理程序. 

### 中断与异常

在ARM.v8体系结构中, 中断是更笼统的术语：异常. 异常有4种

* **同步异常** 这种类型的异常总是由当前执行的指令引起. 例如, 您可以使用 `str` 指令将一些数据存储在不存在的内存位置. 在这种情况下, 将生成同步异常. 同步异常也可以用于生成 `软件中断`. 软件中断是由 `svc` 指令有意产生的同步异常. 我们将在第5课中使用该技术来实现系统调用. 
* **IRQ(中断请求)** 这些是正常的中断. 它们始终是异步的, 这意味着它们与当前执行的指令无关. 与同步异常相反, 它们始终不是由处理器本身生成的, 而是由外部硬件生成的. 
* **FIQ(快速中断请求)** 这种类型的异常称为“快速中断”, 仅出于优先处理异常的目的而存在. 可以将某些中断配置为“正常”, 将其他中断配置为“快速”. 快速中断将首先发出信号, 并将由单独的异常处理程序处理.  `Linux`不使用快速中断, 我们也不会这样做. 
* **SError(系统错误)** 像`IRQ`和`FIQ`一样, `SError`异常是异步的, 由外部硬件生成. 与 `IRQ` 和 `FIQ` 不同, `SError` 始终表示某种错误情况.  您可以在[这里](https://community.arm.com/processors/f/discussions/3205/re-what-is-serror-detailed-explanation-is-required)找到一个示例来说明何时可以使用`SError`产生中断. 

### 异常向量

每个异常类型都需要有自己的处理程序. 另外, 应该为每个不同的执行状态(在其中生成异常)定义单独的处理程序. 从异常处理的角度来看, 有4种执行状态很有趣. 如果我们在EL1工作, 则这些状态可以定义如下：

1. **EL1t** 与EL0共享堆栈指针时, `EL1`发生异常. 当 `SPSel` 寄存器的值为 `0` 时, 就会发生这种情况. 
1. **EL1h** 为EL1分配了专用堆栈指针时, EL1发生了异常. 这意味着 `SPSel` 拥有值 `1`, 这是我们当前正在使用的模式. 
1. **EL0_64** 以64位模式执行的EL0产生异常. 
1. **EL0_32** 以32位模式执行的EL0产生异常. 

总共, 我们需要定义16个异常处理程序(4个异常级别乘以4个执行状态). 一个保存所有异常处理程序地址的特殊结构称为 *exception vector table* 或 *vector table*. 向量表的结构在[AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/ca/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile) 第1876页的 `表D1-7向量与向量表基址的向量偏移量` 中定义. 您可以将向量表视为异常向量的数组, 其中每个异常向量(或处理程序)是负责处理特定异常的连续指令序列. 因此, 对于来自 `AArch64-参考手册` 的 `表D1-7`, 每个异常向量最多可以占用`0x80` 字节. 这虽然不多, 但是没有人阻止我们从异常向量跳转到其他内存位置. 

我认为通过一个示例, 所有这些都将更加清晰, 因此现在该是时候看看RPI-OS中如何实现异常向量了. 与异常处理相关的所有内容都在 [entry.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S) 中进行了定义立即开始进行检查. 

第一个有用的宏称为[ventry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L12), 它用于创建向量表中的条目. 

```
    .macro    ventry    label
    .align    7
    b    \label
    .endm
```

您可能会从该定义中推断出, 我们不会在异常向量内部处理异常, 而是跳转到为宏提供的标签, 它是 `label` 参数. 我们需要 `.align 7` 指令, 因为所有异常向量都应位于彼此偏移的 `0x80` 字节上. 

向量表在[此处](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L64) 定义, 它由16个清单组成定义. 现在, 我们只对处理来自`EL1h`的`IRQ`感兴趣, 但是我们仍然需要定义所有`16`个处理程序. 这不是因为某些硬件要求, 而是因为我们希望看到有意义的错误消息, 以防出现问题. 所有不应该在正常流程中执行的处理程序都具有无效的后缀, 并使用[handle_invalid_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S＃L3)宏. 让我们看看如何定义此宏. 

```
    .macro handle_invalid_entry type
    kernel_entry
    mov    x0, #\type
    mrs    x1, esr_el1
    mrs    x2, elr_el1
    bl    show_invalid_entry_message
    b    err_hang
    .endm
```

在第一行中, 您可以看到使用了另一个宏：`kernel_entry`. 我们将在短期内讨论. 

然后我们调用[show_invalid_entry_message](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L34)并为其准备3个参数. 第一个参数是可以采用[these](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/entry.h#L6)值之一的异常类型.  它准确地告诉我们执行了哪个异常处理程序. 

第二个参数是最重要的参数, 称为 `ESR`, 代表异常综合征寄存器. 该参数取自 `esr_el1` 寄存器, 该寄存器在 `AArch64-Reference-Manual` 的第2431页中进行了描述. 该寄存器包含有关导致异常的原因的详细信息. 

第三个参数主要在同步异常的情况下很重要. 它的值取自我们熟悉的 `elr_el1` 寄存器, 其中包含生成异常时已执行的指令的地址. 对于同步异常, 这也是导致异常的指令. 

在`show_invalid_entry_message`函数将所有这些信息打印到屏幕之后, 我们将处理器置于无限循环中, 因为我们无能为力了. 

### 保存寄存器状态

异常处理程序完成执行后, 我们希望所有通用寄存器具有与生成异常之前相同的值. 如果我们不实现这种功能, 则与当前正在执行的代码无关的中断可能会不可预测地影响该代码的行为. 这就是为什么在生成异常后我们要做的第一件事就是保存处理器状态.  这是在[kernel_entry](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S)宏中完成的.  这个宏非常简单：它只将寄存器`x0-x30`存储到堆栈中. 还有一个相应的宏[kernel_exit](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/entry.S#L37), 在异常处理程序完成执行.  `kernel_exit`通过复制回`x0-x30`寄存器的值来恢复处理器状态.  它还执行 `eret` 指令, 这使我们返回到正常的执行流程. 顺便说一句, 通用寄存器并不是执行异常处理程序之前唯一需要保存的内容, 但是对于我们现在的简单内核而言, 这已经足够了. 在后面的课程中, 我们将为`kernel_entry`和`kernel_exit`宏添加更多功能. 

### 设置向量表

好的, 现在我们准备了向量表, 但是处理器不知道它的位置, 因此无法使用它. 为了使异常处理有效, 我们必须将 `vbar_el1`(向量基址寄存器)设置为向量表地址. 这是在[此处](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.S#L2)完成的. 

```
.globl irq_vector_init
irq_vector_init:
    adr    x0, vectors        // load VBAR_EL1 with virtual
    msr    vbar_el1, x0        // vector table address
    ret
```

### 屏蔽/取消屏蔽中断

我们需要做的另一件事是取消屏蔽所有类型的中断. 让我解释一下“取消屏蔽”中断的含义. 有时有必要告诉您, 特定的代码段绝不能被异步中断拦截. 想象一下, 例如, 如果在`kernel_entry`宏的中间发生中断, 会发生什么？在这种情况下, 处理器状态将被覆盖并丢失. 这就是为什么每当执行异常处理程序时, 处理器都会自动禁用所有类型的中断. 这称为“遮罩”, 如果需要, 也可以手动完成. 

许多人错误地认为必须在异常处理程序的整个过程中屏蔽中断. 这是不正确的-在保存处理器状态后取消屏蔽中断是完全合法的, 因此嵌套嵌套的中断也是合法的. 我们现在不打算这样做, 但是这是要记住的重要信息. 

[以下两个功能](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.S#L7-L15)负责屏蔽和取消屏蔽中断. 

```
.globl enable_irq
enable_irq:
    msr    daifclr, #2
    ret

.globl disable_irq
disable_irq:
    msr    daifset, #2
        ret
```

ARM处理器状态有4位, 负责保持不同类型中断的屏蔽状态. 这些位定义如下. 

* **D**  屏蔽调试异常. 这些是同步异常的一种特殊类型. 出于明显的原因, 不可能屏蔽所有同步异常, 但是使用单独的标志可以屏蔽调试异常很方便. 

* **A** 掩盖`SErrors`. 之所以称为 `A`, 是因为有时将 `SErrors` 称为异步中止. 
* **I** 口罩 `IRQs`
* **F** Masks `FIQs`

现在您可能会猜出为什么负责更改中断屏蔽状态的寄存器称为 `daifclr` 和 `daifset` . 这些寄存器在处理器状态下设置和清除中断屏蔽状态位. 

您可能想知道的最后一件事是为什么我们在两个函数中都使用常量值 `2`？这是因为我们只想设置并清除第二个(I)位. 

### 配置中断控制器

设备通常不直接中断处理器：相反, 它们依靠中断控制器来完成工作. 中断控制器可用于启用/禁用硬件发送的中断. 我们还可以使用中断控制器来确定哪个设备产生了中断.  Raspberry PI具有自己的中断控制器, 该控制器在 [BCM2837 ARM 外设手册](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2) 的第109页上进行了描述. 

Raspberry Pi中断控制器具有3个寄存器, 用于保存所有类型的中断的启用/禁用状态. 目前, 我们仅对计时器中断感兴趣, 可以使用[ENABLE_IRQS_1](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/include/外设/irq.h#L10)寄存器, 在 `BCM2837 ARM外设手册` 的第116页上进行了介绍.  根据文档, 中断分为2个存储区. 第一个存储区由中断 `0-31` 组成, 可以通过将寄存器 `ENABLE_IRQS_1` 的不同位置1来启用或禁用这些中断.  最后32个中断还有一个对应的寄存器 - `ENABLE_IRQS_2`和一个控制一些常见中断以及ARM本地中断的寄存器 - `ENABLE_BASIC_IRQS`(在本课程的下一章中将讨论ARM本地中断).  但是, 《外围设备手册》有很多错误, 其中之一与我们的讨论直接相关.  外围设备中断表(在手册第113页上进行了说明)应在 `0-3` 行包含4个来自系统定时器的中断.  从逆向工程Linux源代码并阅读[其他一些资源](http://embedded-xinu.readthedocs.io/en/latest/arm/rpi/BCM2835-System-Timer.html), 我能够弄清楚该计时器中断0和2被保留并由GPU使用, 中断1和3可以用于任何其他目的.  因此, 这是启用系统计时器IRQ编号1的[功能](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L29). 

```
void enable_interrupt_controller()
{
    put32(ENABLE_IRQS_1, SYSTEM_TIMER_IRQ_1);
}
```

### 通用IRQ处理程序

从前面的讨论中, 您应该记住, 我们只有一个异常处理程序, 负责处理所有的 `IRQ` . 此处理程序在[此处](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L39) 中定义. 

```
void handle_irq(void)
{
    unsigned int irq = get32(IRQ_PENDING_1);
    switch (irq) {
        case (SYSTEM_TIMER_IRQ_1):
            handle_timer_irq();
            break;
        default:
            printf("Unknown pending irq: %x\r\n", irq);
    }
}
```

在处理程序中, 我们需要一种方法来确定哪个设备负责产生中断. 中断控制器可以帮助我们完成此工作：它具有`IRQ_PENDING_1`寄存器, 该寄存器保存中断`0-31`的中断状态. 使用该寄存器, 我们可以检查当前中断是由计时器还是由其他设备产生的, 并调用设备特定的中断处理程序. 注意, 多个中断可以同时挂起. 这就是每个设备特定的中断处理程序必须确认已完成对中断的处理的原因, 只有在`IRQ_PENDING_1`中的该中断挂起位被清除后, 该原因才会被清除. 由于相同的原因, 对于准备投入生产的`OS`, 您可能希望在循环中将开关构造包装在中断处理程序中：这样, 您将能够在单个处理程序执行期间处理多个中断. 

### Timer initialization

Raspberry Pi系统计时器是一个非常简单的设备. 它具有一个计数器, 该计数器在每个时钟滴答之后将其值增加1. 它还具有连接到中断控制器的4条中断线(因此它可以生成4个不同的中断)和4个相应的比较寄存器. 当计数器的值等于存储在比较寄存器之一中的值时, 将触发相应的中断. 这就是为什么在我们能够使用系统定时器中断之前, 我们需要使用一个非零值初始化比较寄存器之一, 该值越大-越晚生成中断. 这是在[timer_init](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/timer.c#L8) 函数中完成的. 

```
const unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init ( void )
{
    curVal = get32(TIMER_CLO);
    curVal += interval;
    put32(TIMER_C1, curVal);
}
```

第一行读取当前计数器值, 第二行增加当前计数器值, 第三行为中断编号1设置比较寄存器的值. 通过操作“`interval` 值, 您可以调整第一次定时器中断的产生时间. 

### 处理计时器中断

最后, 我们到达了计时器中断处理程序. 实际上很简单. 

```
void handle_timer_irq( void )
{
    curVal += interval;
    put32(TIMER_C1, curVal);
    put32(TIMER_CS, TIMER_CS_M1);
    printf("Timer iterrupt received\n\r");
}
```

在这里, 我们首先更新比较寄存器, 以便在相同的时间间隔后产生下一个中断. 接下来, 我们通过将1写入“`TIMER_CS` 寄存器来确认中断. 在文档 `TIMER_CS` 中称为 `计时器控制/状态` 寄存器. 该寄存器的位[0：3]可用于确认来自4条可用中断线之一的中断. 

### 结论

您可能要看的最后一件事是[kernel_main](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/kernel.c#L7) 功能, 其中协调了所有先前讨论的功能. 编译并运行示例后, 应在中断发生后输出 `收到计时器中断`消息. 请尝试自己动手做, 不要忘记仔细检查代码并进行试验. 

##### 上一页

2.3 [Processor initialization: Exercises](../../docs/lesson02/exercises.md)

##### 下一页

3.2 [Interrupt handling: Low-level exception handling in Linux](../../docs/lesson03/linux/low_level-exception_handling.md)
