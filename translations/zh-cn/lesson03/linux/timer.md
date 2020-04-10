## 3.4: Timers

通过检查全局中断控制器, 我们完成了上一章. 我们能够一直跟踪计时器中断的路径, 直到[bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) 功能. 下一步是查看计时器驱动程序如何处理此中断. 但是, 在我们执行此操作之前, 您需要熟悉一些与计时器功能相关的重要概念.  所有这些都在[official kernel documentation](https://github.com/torvalds/linux/blob/v4.14/Documentation/timers/timekeeping.txt), 我强烈建议您阅读本文档. 但是对于那些忙于阅读它的人, 我可以对所提到的概念提供自己的简短解释. 

1. **Clock sources** 每次您需要确切地确定现在是几点, 您都在使用时钟源框架. 通常, 时钟源被实现为单调的原子n位计数器, 该计数器从0计数到`2^(n-1)`, 然后回绕到0并重新开始. 时钟源还提供了将计数器转换为纳秒级值的方法. 
1. **Clock events** 引入此抽象是为了允许任何人订阅计时器中断. 时钟事件框架将下一个事件的设计时间作为输入, 并以此为基础计算计时器硬件寄存器的适当值. 
1. **sched_clock()** 此函数返回自系统启动以来的纳秒数. 通常通过直接读取定时器寄存器来实现. 经常调用此函数, 应针对性能进行优化. 

在下一节中, 我们将了解如何使用系统定时器来实现时钟源, 时钟事件和`sched_clock`功能. 

### BCM2835 系统计时器.

像往常一样, 我们通过在设备树中找到其位置来开始探索特定设备.  系统计时器节点已定义 [这里](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57). 您可以暂时打开此定义, 因为我们将多次引用它. 

接下来, 我们需要使用 `compatible` 属性来找出相应驱动程序的位置.  可以找到驱动程序在 [这里](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c). 我们要看的第一件事是 [bcm2835_timer](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L42) 结构体.

```
struct bcm2835_timer {
    void __iomem *control;
    void __iomem *compare;
    int match_mask;
    struct clock_event_device evt;
    struct irqaction act;
};
```

该结构包含驱动程序运行所需的所有状态.  `控制` 和`比较` 字段保存相应的内存映射寄存器的地址, `match_mask` 用于确定我们将使用的4个可用定时器中断中的哪一个, `evt` 字段包含传递给时钟的结构事件框架和`act` 是一个irq动作, 用于将当前驱动程序与中断控制器连接. 

接下来, 我们将看一下[bcm2835_timer_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L83), 它是驱动程序初始化功能. 它很大, 但没有一开始就想的那么难. 


```cpp
static int __init bcm2835_timer_init(struct device_node *node)
{
    void __iomem *base;
    u32 freq;
    int irq, ret;
    struct bcm2835_timer *timer;

    base = of_iomap(node, 0);
    if (!base) {
        pr_err("Can't remap registers\n");
        return -ENXIO;
    }

    ret = of_property_read_u32(node, "clock-frequency", &freq);
    if (ret) {
        pr_err("Can't read clock-frequency\n");
        goto err_iounmap;
    }

    system_clock = base + REG_COUNTER_LO;
    sched_clock_register(bcm2835_sched_read, 32, freq);

    clocksource_mmio_init(base + REG_COUNTER_LO, node->name,
        freq, 300, 32, clocksource_mmio_readl_up);

    irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
    if (irq <= 0) {
        pr_err("Can't parse IRQ\n");
        ret = -EINVAL;
        goto err_iounmap;
    }

    timer = kzalloc(sizeof(*timer), GFP_KERNEL);
    if (!timer) {
        ret = -ENOMEM;
        goto err_iounmap;
    }

    timer->control = base + REG_CONTROL;
    timer->compare = base + REG_COMPARE(DEFAULT_TIMER);
    timer->match_mask = BIT(DEFAULT_TIMER);
    timer->evt.name = node->name;
    timer->evt.rating = 300;
    timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
    timer->evt.set_next_event = bcm2835_time_set_next_event;
    timer->evt.cpumask = cpumask_of(0);
    timer->act.name = node->name;
    timer->act.flags = IRQF_TIMER | IRQF_SHARED;
    timer->act.dev_id = timer;
    timer->act.handler = bcm2835_time_interrupt;

    ret = setup_irq(irq, &timer->act);
    if (ret) {
        pr_err("Can't set up timer IRQ\n");
        goto err_iounmap;
    }

    clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);

    pr_info("bcm2835: system timer (irq = %d)\n", irq);

    return 0;

err_iounmap:
    iounmap(base);
    return ret;
}
```
Now let's take a closer look at this function.

```
    base = of_iomap(node, 0);
    if (!base) {
        pr_err("Can't remap registers\n");
        return -ENXIO;
    }
```

它从映射内存寄存器开始并获得寄存器基地址. 您应该已经熟悉此部分. 

```
    ret = of_property_read_u32(node, "clock-frequency", &freq);
    if (ret) {
        pr_err("Can't read clock-frequency\n");
        goto err_iounmap;
    }

    system_clock = base + REG_COUNTER_LO;
    sched_clock_register(bcm2835_sched_read, 32, freq);
```

接下来, 初始化“ sched_clock”子系统.  sched_clock每次执行时都需要访问计时器计数器寄存器, 并且 [bcm2835_sched_read](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L52) 作为第一个参数传递来协助完成此任务. 第二个参数对应于计时器计数器的位数(在我们的示例中为32). 位的数量用于计算计数器将换为0的时间. 最后一个参数指定计时器频率 - 它用于将计时器计数器的值转换为纳秒.  计时器频率在设备树中的以下位置定义 [这](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L65) 行.

```
    clocksource_mmio_init(base + REG_COUNTER_LO, node->name,
        freq, 300, 32, clocksource_mmio_readl_up);
```

下一行初始化时钟源框架.  [clocksource_mmio_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/mmio.c#L52) 根据内存映射寄存器初始化一个简单的时钟源.  在某些方面, 时钟源框架复制了 `sched_clock` 的功能, 并且需要访问相同的3个基本参数. 

* 计时器计数器寄存器的位置. 
* 计数器中的有效位数. 
* 计时器频率. 

另外3个参数包括时钟源的名称, 其评级(用于对时钟源设备进行评级)以及可以读取定时器计数器寄存器的功能. 

```
    irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
    if (irq <= 0) {
        pr_err("Can't parse IRQ\n");
        ret = -EINVAL;
        goto err_iounmap;
    }
```

此代码段用于查找Linux irq编号, 该编号对应于第三个计时器中断(编号3硬编码为 [DEFAULT_TIMER](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L108) 不变). 快速提醒一下：Raspberry Pi系统计时器具有4组独立的计时器寄存器, 此处使用第三个.   如果回到设备树, 您可以找到 [打断](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L60) 属性. 此属性描述设备支持的所有中断, 以及这些中断如何映射到中断控制器线路.  它是一个数组, 其中每个项代表一个中断. 项目的格式特定于中断控制器. 在我们的例子中, 每个项目由2个数字组成：第一个指定一个中断存储区, 第二个指定一个中断存储区 - 银行内部的中断号.  [irq_of_parse_and_map](https://github.com/torvalds/linux/blob/v4.14/drivers/of/irq.c#L41) 读取的值 `interrupts` 属性, 然后它使用第二个参数来查找我们感兴趣的支持的中断, 并为请求的中断返回Linux irq号. 

```
    timer = kzalloc(sizeof(*timer), GFP_KERNEL);
    if (!timer) {
        ret = -ENOMEM;
        goto err_iounmap;
    }
```

在这里分配了 `bcm2835_timer` 结构的内存. 

```
    timer->control = base + REG_CONTROL;
    timer->compare = base + REG_COMPARE(DEFAULT_TIMER);
    timer->match_mask = BIT(DEFAULT_TIMER);
```

接下来, 计算控制和比较寄存器的地址, 并将 `match_mask` 设置为 `DEFAULT_TIMER` 常量. 

```
    timer->evt.name = node->name;
    timer->evt.rating = 300;
    timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
    timer->evt.set_next_event = bcm2835_time_set_next_event;
    timer->evt.cpumask = cpumask_of(0);
```

在此代码段中, [clock_event_device](https://github.com/torvalds/linux/blob/v4.14/include/linux/clockchips.h#L100) 被初始化. 这里最重要的属性是`set_next_event`, 它指向[bcm2835_time_set_next_event](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L57) 函数. 时钟事件框架调用此函数以调度下一个中断.  `bcm2835_time_set_next_event` 很简单 - 它更新比较寄存器, 以便在需要的时间间隔后安排中断.  这类似于我们所做的 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/timer.c#L17) 适用于RPi OS. 

```
    timer->act.flags = IRQF_TIMER | IRQF_SHARED;
    timer->act.dev_id = timer;
    timer->act.handler = bcm2835_time_interrupt;
```

接下来, irq动作被初始化. 这里最重要的属性是`handler`, 它指向[bcm2835_time_interrupt](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L67) -  这是在触发中断后调用的函数. 如果看一看, 您会发现它会将所有工作重定向到由时钟事件框架注册的事件处理程序. 我们将在一段时间后检查此事件处理程序. 

```
    ret = setup_irq(irq, &timer->act);
    if (ret) {
        pr_err("Can't set up timer IRQ\n");
        goto err_iounmap;
    }
```

配置irq操作后, 它将被添加到计时器中断的irq操作列表中. 

```
    clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);
```

最后, 通过调用初始化时钟事件框架 [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504). `evt` 结构和计时器频率作为前两个参数传递. 后2个参数仅在 `单次` 计时器模式下使用, 与我们当前的讨论无关. 

现在, 我们一直跟踪到计时器中断的路径, 一直到`bcm2835_time_interrupt`函数为止, 但是我们仍然没有找到完成实际工作的地方. 在下一部分中, 我们将更深入地研究并发现中断进入时钟事件框架时如何处理. 

### 在时钟事件框架中如何处理中断

在上一节中, 我们看到了处理计时器中断的实际工作已外包给时钟事件框架. 这是在 [以下](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L74) 几行完成.

```
        event_handler = ACCESS_ONCE(timer->evt.event_handler);
        if (event_handler)
            event_handler(&timer->evt);
```

现在我们的目标是弄清楚到底是设置了`event_handler`以及调用后会发生什么. 

[clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) 函数是开始探索的好地方, 因为这是配置时钟事件框架的地方, 并且, 如果我们遵循此函数的逻辑, 最终我们应该找到如何设置 `event_handler`. 

现在, 让我向您展示函数调用链, 这些函数调用将我们引导至所需的位置. 

1. [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) 这是顶级初始化功能. 
1. [clockevents_register_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L449) 在此功能中, 计时器被添加到时钟事件设备的全局列表中. 
1. [tick_check_new_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L300)  此功能检查当前设备是否适合用作 `刻度设备`.  如果是, 则将使用此类设备生成定期的滴答声, 内核的其余部分将使用这些滴答声来完成需要定期执行的所有工作. 
1. [tick_setup_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L177) 此功能启动设备配置. 
1. [tick_setup_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L144) 这是设备配置为周期性抽动的地方. 
1. [tick_set_periodic_handler](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-broadcast.c#L432)  终于我们到达了分配处理程序的地方！

如果看一下调用链中的最后一个函数, 您会发现Linux使用不同的处理程序, 具体取决于是否启用了广播.  Tick广播用于唤醒空闲的CPU, 您可以阅读有关它的更多信息在 [这里](https://lwn.net/Articles/574962/) 但是我们将忽略它, 而专注于更通用的滴答处理程序. 

一般情况下 [tick_handle_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L99) 接着 [tick_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L79) 函数被调用. 后面的正是我们感兴趣的功能. 让我在这里复制其内容. 

```cpp
/*
 * Periodic tick
 */
static void tick_periodic(int cpu)
{
    if (tick_do_timer_cpu == cpu) {
        write_seqlock(&jiffies_lock);

        /* Keep track of the next tick event */
        tick_next_period = ktime_add(tick_next_period, tick_period);

        do_timer(1);
        write_sequnlock(&jiffies_lock);
        update_wall_time();
    }

    update_process_times(user_mode(get_irq_regs()));
    profile_tick(CPU_PROFILING);
}
```

此功能完成了一些重要的事情：

1. `tick_next_period` 被用于计算以便可以安排下一个滴答事件. 
1.  [do_timer](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timekeeping.c#L2200) 被调用, 负责设置“jiffies”.   `jiffies`是自上次系统重新启动以来的滴答声.  `jiffies` 在不需要纳秒精度的情况下, 可以与 `sched_clock`函数相同的方式使用. 
1. [update_process_times](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timer.c#L1583) 被调用. 在这里, 当前执行的进程有机会进行需要定期执行的所有工作. 这项工作包括, 例如, 运行本地进程计时器, 或者最重要的是, 将滴答事件通知调度程序. 

### 结论

现在您可以看到普通定时器中断的时间有多长, 但是我们从头到尾一直遵循它. 最重要的事情之一是, 我们终于到达了调度程序的调用位置. 调度程序是任何操作系统中最关键的部分之一, 它严重依赖计时器中断. 因此, 现在, 当我们看到调度程序功能在何处触发时, 就该讨论其实现了, 这是我们在下一课中要做的事情. 

##### 上一页

3.3 [中断处理：中断控制器](../../../docs/lesson03/linux/interrupt_controllers.md)

##### 下一页

3.5 [中断处理：练习](../../../docs/lesson03/exercises.md)
