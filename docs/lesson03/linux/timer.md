## Lesson 3: Timers in linux.

We finished last chapter by examining global interrupt controller and were able to trace the path of a timer interrup all the way up to the [bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) function. Next logical stop is to see how this interrupt is handled by the timer driver. However, before we will be able to do this, you need to familiarize yoursef with a few important concepts related to timer functionality. All of them are explained in the {official kernel documentation}(https://github.com/torvalds/linux/blob/v4.14/Documentation/timers/timekeeping.txt), and I strongly advice you to read this document. But for those who are too bysy to read it I I can provide my own breff explanation of the mentioned concepts.

1. **Clock sources** Each time you need to find out exactly what time is is now you are using clouck source. Typically the clock source is implemented as a monotonic, atomic counter which will provide n bits which count from 0 to 2^(n-1) and then wraps around to 0 and start over. The clock source also provides means to translate the provided counter into a nanosecond value.
1. **Clock events** This abstruction is provided to allow anybody to subscribe on timer interrupts. Clock events takes designed time of the events as an input and based on it calculates appropriate values for timer hardware registers.
1. **sched_clock()** This function returns the number of nanoseconds since the system was started. It usually does so by directly reading timer registers. 

In the next section we are going to see how system timer is used to implement clock sources, clock events and sched_clock functionality.

### BCM2835 System Timer.

As usual, we atart exploration of a particular divice with finding its location in the device tree. System timer node is defined [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57). You can keep this definition open for a while, because we are going to reference it several times.

Next we need to use `compatible` property to figure out the location of the corresponding driver. The driver can be found [herea](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c). The first thing we are going to look at is [bcm2835_timer](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L42) structure.

```
struct bcm2835_timer {
	void __iomem *control;
	void __iomem *compare;
	int match_mask;
	struct clock_event_device evt;
	struct irqaction act;
};
```

This structure contains all information needed for the driver to function. `control` and `compare` fields holds the addresses of coresponding memory mapped registers, `match_mask` is used to determine wich of the 4 available times we expect to generate interrupts, `evt` field contains a structure that is passed to clock events framework and `act` is an irq action that is used to connect current driver with interrupt controller. 

Next we are going to look at [bcm2835_timer_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L83) wich is the driver initialization function. It starts with  mapping memory registers and obtaining register base address.

```
	base = of_iomap(node, 0);
	if (!base) {
		pr_err("Can't remap registers\n");
		return -ENXIO;
	}
```

Next, `sched_clock` subsustem is initialized.

```
	ret = of_property_read_u32(node, "clock-frequency", &freq);
	if (ret) {
		pr_err("Can't read clock-frequency\n");
		goto err_iounmap;
	}

	system_clock = base + REG_COUNTER_LO;
```

`sched_clock` need to access timer counter registers each time it is executed and [bcm2835_sched_read](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L52) is passed as the first argument to assist with this task. Second argument correspond to the number of bits that timer sounter has (in our case it is 32). the number of bits is used to calculate how soon the counter is going to wrap to 0. The last argument specifies timer frequency - it is used to convert values from the timer counter to nanoseconds. Timer frequency is specified in the device tree at [this](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L65) line.

Next line initializes clock source framwork.

```
	clocksource_mmio_init(base + REG_COUNTER_LO, node->name,
		freq, 300, 32, clocksource_mmio_readl_up);
```

[clocksource_mmio_init](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/mmio.c#L52) initializes simple clocksource based on memory mapped registers. Clocksource framwork in some aspects duplicates functionality of `sched_clock` and it need access to the same 3 basic parameters: location of the timer counter register, number of valid bits in the counter and timer frequency. Another 3 parameters include name of the clocksource, its rating, wich is used to rate cloucksource devices, and function that can read timer counter register.

```
	irq = irq_of_parse_and_map(node, DEFAULT_TIMER);
	if (irq <= 0) {
		pr_err("Can't parse IRQ\n");
		ret = -EINVAL;
		goto err_iounmap;
	}
```

This code snippet is used to find linux interrupt number corresponding to timer interrupt number 3 (This number is hardcoded as [DEFAULT_TIMER](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L108) constant). If you go back to device tree you can find [interrupts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L60) property. This property describes all interrupts, supported by a device, and how those interrupts are mapped to interrupt controller lines and it is an array of items, were each item describes one interrupt. The format of the items is specific to interrupt controller. In our case each item consist of 2 numbers: first of them specifies interrupt bank and second interrupt number. [irq_of_parse_and_map](https://github.com/torvalds/linux/blob/v4.14/drivers/of/irq.c#L41) reads the value of `interrupts` property, than it uses second argument to find which of the supported interrupts we are interested it and then returns linux irq number for required interrupt.

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

Then locations of the control and compare registers are calculated and `match_mask` is set to `DEFAULT_TIMER` constant.

```
	timer->evt.name = node->name;
	timer->evt.rating = 300;
	timer->evt.features = CLOCK_EVT_FEAT_ONESHOT;
	timer->evt.set_next_event = bcm2835_time_set_next_event;
	timer->evt.cpumask = cpumask_of(0);
```

In this code snippet [clock_event_device](https://github.com/torvalds/linux/blob/v4.14/include/linux/clockchips.h#L100) struct is initialized. The most important property here is `set_next_event` wich points to [bcm2835_time_set_next_event](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L57)  function. This function is called by clock events framework to schedule next interrupt. `bcm2835_time_set_next_event` is very simple - it updates compare register so that interrupt will be scheduled after a desied interval. This is analogaus to what we did [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/timer.c#L17) for RPi OS.

```
	timer->act.flags = IRQF_TIMER | IRQF_SHARED;
	timer->act.dev_id = timer;
	timer->act.handler = bcm2835_time_interrupt;
```

Next irq action is initialized. The most important property here is `handler` wich points to [bcm2835_time_interrupt](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L67) -  this is the function that is called after interrupt is fired. If you take a look at it you will see that it redirects all work to the event handlers, registered by clock events framework. We are going to examine this event handler in a while.

```
	ret = setup_irq(irq, &timer->act);
	if (ret) {
		pr_err("Can't set up timer IRQ\n");
		goto err_iounmap;
	}
```

After irq action is configured it is actually added to the list of irq actions of the timer interrupt. 

```
	clockevents_config_and_register(&timer->evt, freq, 0xf, 0xffffffff);
```

And finally clock events framework is initialized by calling [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504). `evt` structure and timer frequency are passed as first 2 arguments. Last 2 arguments are used only in timer "one-shot" mode and are not relevant to our current discussion.

Now, we have traced the path of timer interrupt all the way up to the `bcm2835_time_interrupt` function, but we still didn't find the place were actuall work is done. In the next section we are going to dig even deper and find out how an interrupt is processed when it enters clock events framework. 

### How an interrupt is processed inside clock events framework.

n the previous section we have seen that the actuall work of handling timer interrupt is outsourced to clock events framework. This is done in the [following](https://github.com/torvalds/linux/blob/v4.14/drivers/clocksource/bcm2835_timer.c#L74) few lines.

```
		event_handler = ACCESS_ONCE(timer->evt.event_handler);
		if (event_handler)
			event_handler(&timer->evt);
```

Now our goal will be to figure out were exactly `event_handler`  is set and what happens after it is called.

[clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) function is a good place to start the exploration, because this is the place were clock events framework is configured and if we follow the logic of this function eventually we should find how `event_handler` is set. 

Now let me show you the chain of function calls that leads us to the place we need.

1. [clockevents_config_and_register](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L504) - this is the top level initialization function.
1. [clockevents_register_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/clockevents.c#L449) - in this function clock event device is added to the global list of such devices.
1. [tick_check_new_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L300)  - this function checks whether current device is a good candidate for using is as a "tick device". If yes, such device will be used to generate periodic tics that rest of the kernel will use to do all work that need to be done on a regular basis. 
1. [tick_setup_device](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L177) - this function starts device configuration.
1. [tick_setup_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L144) - this is the place were device is configured for periodic tics.
1. [tick_set_periodic_handler](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-broadcast.c#L432)  - finally we reached the place were handler is assigned!

If you take a look at the last function in the call chain, you will see that linux uses different handlers in case whether broadcast is enabled or not. Tick broadcast is used to awake idle CPUs, you can read more about it [here](https://lwn.net/Articles/574962/) but we are going to ignore it and concentrate on a more general tick handler instead.

In general case [tick_handle_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L99) and then [tick_periodic](https://github.com/torvalds/linux/blob/v4.14/kernel/time/tick-common.c#L79) function are called. The later one is exactly the function that we are interested in! Let me copy it content here.

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

A few important things are done here:

1. `tick_next_period` is calculated so the next tick event can be scheduled.
1.  [do_timer](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timekeeping.c#L2200) is called, wich is responsible for setting 'jiffies' - number of tick sinse system reboot. `jiffies` can be used in the same way as `sched_clock` in cases were you don't need nanosecond preceigion.
1. [update_process_times](https://github.com/torvalds/linux/blob/v4.14/kernel/time/timer.c#L1583) is called. This is the place were currently executing process is given a chance to do all works that needed to be done periodically. This work includes, for example, running local process timers, or, most importantly, notifying scheduler about tick event.

### Conclusion

Now you see how long is that way of an ordinary timer interrupt, but we followed it all from the beggining to the very end. One of the things that are most important is that we finally reached the place were shceduler is called. Scheduler is one of the most important part of any operating sysem and it also relies havily on timer interrupts. So now, when we've een the place were scheduler functionality is triggered its timer to stop talking about timers and interrupts and discuss scheduler implementation instead - that is something we are going to do in the next lesson.
