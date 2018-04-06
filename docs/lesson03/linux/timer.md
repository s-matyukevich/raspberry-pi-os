## 3.4: Timers

We finished the last chapter by examining global interrupt controller and were able to trace the path of a timer interrupt all the way up to the [bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) function. Next logical step is to see how the timer driver handles this interrupt. However, before we can do this, you need to familiarize yourself with a few important concepts related to timer functionality. All of them are explained in the [official kernel documentation](https://github.com/torvalds/linux/blob/v4.14/Documentation/timers/timekeeping.txt), and I strongly advise you to read this document. But for those who are too busy to read it, I can provide my own brief explanation of the mentioned concepts.

1. **Clock sources** Each time you need to find out exactly what time it is now you are using clock source. Typically the clock source is implemented as a monotonic, atomic n-bit counter, which counts from 0 to 2^(n-1) and then wraps around to 0 and starts over. The clock source also provides means to translate the counter into a nanosecond value.
1. **Clock events** This abstraction is introduced to allow anybody to subscribe on timer interrupts. Clock events framework takes designed time of the next event as input and, based on it, calculates appropriate values for timer hardware registers.
1. **sched_clock()** This function returns the number of nanoseconds since the system was started. It usually does so by directly reading timer registers. This function is called very frequently and should be optimized for performance.

In the next section, we are going to see how system timer is used to implement clock sources, clock events and sched_clock functionality.

### BCM2835 System Timer.

As usual, we start the exploration of a particular device with finding its location in the device tree. System timer node is defined [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57). You can keep this definition open for a while because we are going to reference it several times.

Next, we need to use `compatible` property to figure out the location of the corresponding driver. The driver can be found [herea](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c). The first thing we are going to look at is [bcm2835_timer](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L42) structure.

```
struct bcm2835_timer {
    void __iomem *control;
    void __iomem *compare;
    int match_mask;
    struct clock_event_device evt;
    struct irqaction act;
};
```

This structure contains all state needed for the driver to function. `control` and `compare` fields holds the addresses of the corresponding memory mapped registers, `match_mask` is used to determine which of the 4 available times we expect to generate interrupts, `evt` field contains a structure that is passed to clock events framework and `act` is an irq action that is used to connect the current driver with interrupt controller. 

Next we are going to look at [bcm2835_timer_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L83) which is the driver initialization function. It is large, but not as difficult as you might think from the beginning.

```
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

It starts with mapping memory registers and obtaining register base address. You should be already familiar with this part.

```
    ret = of_property_read_u32(node, "clock-frequency", &freq);
    if (ret) {
        pr_err("Can't read clock-frequency\n");
        goto err_iounmap;
    }

    system_clock = base + REG_COUNTER_LO;
```

Next, `sched_clock` subsystem is initialized. `sched_clock` need to access timer counter registers each time it is executed and [bcm2835_sched_read](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L52) is passed as the first argument to assist with this task. The second argument corresponds to the number of bits that the timer counter has (in our case it is 32). the number of bits is used to calculate how soon the counter is going to wrap to 0. The last argument specifies timer frequency - it is used to convert values from the timer counter to nanoseconds. Timer frequency is defined in the device tree at [this](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L65) line.

```
    clocksource_mmio_init(base + REG_COUNTER_LO, node->name,
        freq, 300, 32, clocksource_mmio_readl_up);
```

Next line initializes clock source framework. [clocksource_mmio_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/mmio.c#L52) initializes a simple clock source based on memory mapped registers. The clock source framework, in some aspects, duplicates the functionality of `sched_clock` and it needs access to the same 3 basic parameters: 
* The location of the timer counter register.
* The number of valid bits in the counter 
* Timer frequency. 
Another 3 parameters include the name of the clock source, its rating, which is used to rate clock source devices, and function that can read timer counter register.

```
    irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
    if (irq <= 0) {
        pr_err("Can't parse IRQ\n");
        ret = -EINVAL;
        goto err_iounmap;
    }
```

This code snippet is used to find Linux interrupt number corresponding to the timer interrupt number 3 (This number is hardcoded as [DEFAULT_TIMER](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L108) constant). Just a quick reminder: Raspberry Pi system timer has 4 independent set of timer registers, and here the third one is used.  If you go back to the devise tree, you can find [interrupts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L60) property. This property describes all interrupts, supported by a device, and how those interrupts are mapped to interrupt controller lines. It is an array, where each item represents one interrupt. The format of the items is specific to the interrupt controller. In our case, each item consists of 2 numbers: the first one specifies an interrupt bank and the second - interrupt number. [irq_of_parse_and_map](https://github.com/torvalds/linux/blob/v4.14/drivers/of/irq.c#L41) reads the value of `interrupts` property, then it uses the second argument to find which of the supported interrupts we are interested in and returns Linux irq number for the requested interrupt.

```
    timer = kzalloc(sizeof(*timer), GFP_KERNEL);
    if (!timer) {
        ret = -ENOMEM;
        goto err_iounmap;
    }
```

Here memory for `bcm2835_timer` structure is allocated.

```
    timer->control = base + REG_CONTROL;
    timer->compare = base + REG_COMPARE(DEFAULT_TIMER);
    timer->match_mask = BIT(DEFAULT_TIMER);
```

Than memory locations of the control and compare registers are calculated and `match_mask` is set to the `DEFAULT_TIMER` constant.

```
    timer->evt.name = node->name;
    timer->evt.rating = 300;
    timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
    timer->evt.set_next_event = bcm2835_time_set_next_event;
    timer->evt.cpumask = cpumask_of(0);
```

In this code snippet [clock_event_device](https://github.com/torvalds/linux/blob/v4.14/include/linux/clockchips.h#L100) struct is initialized. The most important property here is `set_next_event` wich points to [bcm2835_time_set_next_event](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L57)  function. This function is called by clock events framework to schedule next interrupt. `bcm2835_time_set_next_event` is very simple - it updates compare register so that interrupt will be scheduled after a desied interval. This is analogaus to what we did [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/timer.c#L17) for the RPi OS.

```
    timer->act.flags = IRQF_TIMER | IRQF_SHARED;
    timer->act.dev_id = timer;
    timer->act.handler = bcm2835_time_interrupt;
```

Next, irq action is initialized. The most important property here is `handler`, which points to [bcm2835_time_interrupt](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L67) -  this is the function that is called after an interrupt is fired. If you take a look at it, you will see that it redirects all work to the event handlers, registered by clock events framework. We will examine this event handler in a while.

```
    ret = setup_irq(irq, &timer->act);
    if (ret) {
        pr_err("Can't set up timer IRQ\n");
        goto err_iounmap;
    }
```

After the irq action is configured, it is added to the list of irq actions of the timer interrupt. 

```
    clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);
```

And finally clock events framework is initialized by calling [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504). `evt` structure and timer frequency are passed as first 2 arguments. Last 2 arguments are used only in "one-shot" timer mode and are not relevant to our current discussion.

Now, we have traced the path of a timer interrupt all the way up to the `bcm2835_time_interrupt` function, but we still didn't find the place were the actual work is done. In the next section, we are going to dig even deeper and find out how an interrupt is processed when it enters clock events framework. 

### How an interrupt is processed in the clock events framework

In the previous section, we have seen that the real work of handling timer interrupt is outsourced to clock events framework. This is done in the [following](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L74) few lines.

```
        event_handler = ACCESS_ONCE(timer->evt.event_handler);
        if (event_handler)
            event_handler(&timer->evt);
```

Now our goal will be to figure out were exactly `event_handler`  is set and what happens after it is called.

[clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) function is a good place to start the exploration because this is the place were clock events framework is configured and, if we follow the logic of this function, eventually we should find how `event_handler` is set. 

Now let me show you the chain of function calls that leads us to the place we need.

1. [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) This is the top level initialization function.
1. [clockevents_register_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L449) In this function the time is added to the global list of clock event devices.
1. [tick_check_new_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L300)  This function checks whether hte current device is a good candidate to be used as a "tick device". If yes, such device will be used to generate periodic tics that the rest of the kernel will use to do all work that needs to be done on a regular basis. 
1. [tick_setup_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L177) This function starts device configuration.
1. [tick_setup_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L144) This is the place were device is configured for periodic tics.
1. [tick_set_periodic_handler](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-broadcast.c#L432)  Finally we reached the place were handler is assigned!

If you take a look at the last function in the call chain, you will see that Linux uses different handlers depends on whether broadcast is enabled or not. Tick broadcast is used to awake idle CPUs, you can read more about it [here](https://lwn.net/Articles/574962/) but we are going to ignore it and concentrate on a more general tick handler instead.

In general case [tick_handle_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L99) and then [tick_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L79) functions are called. The later one is exactly the function that we are interested in. Let me copy its content here.

```
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

A few important things are done in this function:

1. `tick_next_period` is calculated so the next tick event can be scheduled.
1.  [do_timer](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timekeeping.c#L2200) is called, which is responsible for setting 'jiffies'. `jiffies` is a number of ticks since the last system reboot. `jiffies` can be used in the same way as `sched_clock` function, in cases when you don't need nanosecond precision.
1. [update_process_times](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timer.c#L1583) is called. This is the place were currently executing process is given a chance to do all works that needed to be done periodically. This work includes, for example, running local process timers, or, most importantly, notifying the scheduler about tick event.

### Conclusion

Now you see how long is the way of an ordinary timer interrupt, but we followed it from the beginning to the very end. One of the things that are the most important, is that we finally reached the place were the shceduler is called. The scheduler is one of the most critical parts of any operating system and it relies heavily on timer interrupts. So now, when we've seen where the scheduler functionality is triggered, its time to discuss its implementation - that is something we are going to do in the next lesson.

