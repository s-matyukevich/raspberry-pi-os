## 3.3: Interrupt controllers

In this chapter, we are going to talk a lot about Linux drivers and how they handle interrupts. We will start with driver initialization code and then take a look at how interrupts are processed after [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44) function.

### Using device tree to find out needed devices and drivers

When implementing interrupts in the RPi OS we have been working with 2 devices: system timer and interrupt controller. Now our goal will be to understand how the same devices work in Linux. The first thing we need to do is to find drivers that are responsible for working with mentioned devices. And in order to find needed drivers we can use [bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts) device tree file. This is the top level device tree file that is specific for Raspberry Pi 3 Model B, it includes other more common device tree files, that are shared between different versions of Raspberry Pi. If you follow the chain of includes and search for `timer` and `interrupt-controller` you can find 4 devices.

1. [Local interrupt controller](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11)
1. [Local timer](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L20)
1. Global interrupt controller. It is defined [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L109) and modified [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L72).
1. [System timer](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57)

Stop, but why do we have 4 devices instead of 2? This requires some explanation, and we will tackle this question in the next section.

### Local vs global interrupt controllers

When you think about interrupt handling in multiprocessor systems, one question you should ask yourself is which core should be responsible for processing a particular interrupt? When an interrupt occurs, are all 4 cores interrupted, or only a single one? Is it possible to route a particular interrupt to a specific core? Another question you may wonder is how one processor can notify another processor if he needs to pass some information to it?

The local interrupt controller is a device that can help you in answering all those questions. It is responsible for the following tasks.

* Configuring which core should receive a specific interrupt.
* Sending interrupts between cores. Such interrupts are called "mailboxes" and allow cores to communicate one with each other.
* Handling interrupts from local timer and performance monitors interrupts (PMU).

The behavior of a local interrupt controller as well as a local timer is documented in [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) manual.

I already mentioned local timer several times. Now you probably wonder why do we need two independent timers in the system? I guess that the primary use-case for using the local timer is when you want to configure all 4 cores to receive timer interrupts simultaneously. If you use system timer you can only route interrupts to a single core.

When working with the RPi OS we didn't work with either local interrupt controller or local timer. That is because by default local interrupt controller is configured in such a way that all external interrupts are sent to the first core, which is exactly what we need. We haven't used local timer because we use system timer instead.

### Local interrupt controller

Accordingly to the [bcm2837.dtsi](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75) the global interrupt controller is a child of the local one. Thus it makes sense to start our exploration with the local controller.

If we need to find a driver that works with a particular device, we should use [compatible](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L12) property. Searching for the value of this property you can easily find that there is a single driver that is compatible with RPi local interrupt controller - here is the corresponding [definition](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L315).

```
IRQCHIP_DECLARE(bcm2836_arm_irqchip_l1_intc, "brcm,bcm2836-l1-intc",
        bcm2836_arm_irqchip_l1_intc_of_init);
```

Now you can probably guess what is the procedure of a driver initialization: the kernel walks through all device definitions in the device tree and for each definition it looks for a matching driver using "compatible" property. If the driver is found, then its initialization function is called. Initialization function is provided during device registration, and in our case this function is [bcm2836_arm_irqchip_l1_intc_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L280).

```
static int __init bcm2836_arm_irqchip_l1_intc_of_init(struct device_node *node,
                              struct device_node *parent)
{
    intc.base = of_iomap(node, 0);
    if (!intc.base) {
        panic("%pOF: unable to map local interrupt registers\n", node);
    }

    bcm2835_init_local_timer_frequency();

    intc.domain = irq_domain_add_linear(node, LAST_IRQ + 1,
                        &bcm2836_arm_irqchip_intc_ops,
                        NULL);
    if (!intc.domain)
        panic("%pOF: unable to create IRQ domain\n", node);

    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPNSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTHPIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTVIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_GPU_FAST,
                     &bcm2836_arm_irqchip_gpu);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_PMU_FAST,
                     &bcm2836_arm_irqchip_pmu);

    bcm2836_arm_irqchip_smp_init();

    set_handle_irq(bcm2836_arm_irqchip_handle_irq);
    return 0;
}
```

The initialization function takes 2 parameters: 'node' and 'parent', both of them are of the type [struct device_node](https://github.com/torvalds/linux/blob/v4.14/include/linux/of.h#L49). `node` represents the current node in the device tree, and in our case it points [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11)  `parent` is a parent node in the device tree hierarchy, and for the local interrupt controller it points to `soc` element (`soc` stands for "system on chip" and it is the simplest possible bus which maps all device registers directly to main memory.).

`node` can be used to read various properties from the current device tree node. For example, the first line of the `bcm2836_arm_irqchip_l1_intc_of_init` function reads the device base address from [reg](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L13) property. However, the process is more complicated than that, because when this function is executed MMU is already enabled, and before we will be able to access some region of physical memory we must map this region to some virtual address. This is exactly what [of_iomap](https://github.com/torvalds/linux/blob/v4.14/drivers/of/address.c#L759) function is doing: it reads `reg` property of the provided node and maps the whole memory region, described by `reg` property, to some virtual memory region.

Next local timer frequency is initialized in [bcm2835_init_local_timer_frequency](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L264) function. There is nothing specific about this function: it just uses some of the registers, described in [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) manual, to initialize local timer.

Next line requires some explanations.

```
    intc.domain = irq_domain_add_linear(node, LAST_IRQ + 1,
                        &bcm2836_arm_irqchip_intc_ops,
                        NULL);
```

Linux assigns a unique integer number to each interrupt, you can think about this number as a unique interrupt ID. This ID is used each time you want to do something with an interrupt (for example, assign a handler, or assign which CPU should handle it). Each interrupt also has a hardware interrupt number. This is usually a number that tells which interrupt line was triggered. `BCM2837 ARM Peripherals manual` has the peripheral interrupt table at page 113 - you can think about an index in this table as a hardware interrupt number. So obviously we need some mechanism to map Linux irq numbers to hardware irq number and vice versa. If there is only one interrupt controller it would be possible to use one to one mapping but in general case a more sophisticated mechanism need to be used. In Linux [struct irq_domain](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L152) implements such mapping. Each interrupt controller driver should create its own irq domain and register all interrupts that it can handle with this domain. Registration function returns Linux irq number that later is used to work with the interrupt.

Next 6 lines are responsible for registering each supported interrupt with the irq domain.

```
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTPNSIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTHPIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_CNTVIRQ,
                     &bcm2836_arm_irqchip_timer);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_GPU_FAST,
                     &bcm2836_arm_irqchip_gpu);
    bcm2836_arm_irqchip_register_irq(LOCAL_IRQ_PMU_FAST,
                     &bcm2836_arm_irqchip_pmu);
```

Accordingly to [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) manual local interrupt controller handles 10 different interrupts: 0 - 3 are interrupts from local timer, 4 - 7 are mailbox interrupts, which are used in interprocess communication, 8 corresponds to all interrupts generated by the global interrupt controller and interrupt 9 is a performance monitor interrupt. [Here](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L67) you can see that the driver defines a set of constants that holds hardware irq number per each interrupt. The registration code above registers all interrupts, except mailbox interrupts, which are registered separately. In order to understand the registration code better lets examine [bcm2836_arm_irqchip_register_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L154) function.

```
static void bcm2836_arm_irqchip_register_irq(int hwirq, struct irq_chip *chip)
{
    int irq = irq_create_mapping(intc.domain, hwirq);

    irq_set_percpu_devid(irq);
    irq_set_chip_and_handler(irq, chip, handle_percpu_devid_irq);
    irq_set_status_flags(irq, IRQ_NOAUTOEN);
}
```

The first line here performs actual interrupt registration. [irq_create_mapping](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdomain.c#L632) takes hardware interrupt number as an input and returns Linux irq number.

[irq_set_percpu_devid](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L849) configures interrupt as "per CPU", so that it will be handled only on the current CPU. This makes perfect sense because all interrupts that we are discussing now are local and they all can be handled only on the current CPU.

[irq_set_chip_and_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L608), as its name suggest, sets irq chip and irq handler. Irq chip is a special struct, which needs to be created by the driver, that has methods for masking and unmasking a particular interrupt. The driver that we are examining right now defines 3 different irq chips: [timer](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L118) chip, [PMU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L134) chip and [GPU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L148) chip, which controls all interrupts generated by the external peripheral devices. Handler is a function that is responsible for processing an interrupt. In this case, the handler is set to generic [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859) function. This handler later will be rewritten by the global interrupt controller driver.

[irq_set_status_flags](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L652) in this particular case sets a flag, indicating that the current interrupt should be enabled manually and should not be enabled by default.

Going back to the `bcm2836_arm_irqchip_l1_intc_of_init` function, there are only 2 calls left. The first one is [bcm2836_arm_irqchip_smp_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L243). Here mailbox interrupts are enabled, allowing processors cores to communicate with each other.

The last function call is extremely important - this is the place where low-level exception handling code is connected to the driver.

```
    set_handle_irq(bcm2836_arm_irqchip_handle_irq);
```

[set_handle_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L46) is defined in architecture specific code and we already encountered this function. From the line above we can understand that [bcm2836_arm_irqchip_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L164) will be called by the low-level exception code. The function itself is listed below.

```
static void
__exception_irq_entry bcm2836_arm_irqchip_handle_irq(struct pt_regs *regs)
{
    int cpu = smp_processor_id();
    u32 stat;

    stat = readl_relaxed(intc.base + LOCAL_IRQ_PENDING0 + 4 * cpu);
    if (stat & BIT(LOCAL_IRQ_MAILBOX0)) {
#ifdef CONFIG_SMP
        void __iomem *mailbox0 = (intc.base +
                      LOCAL_MAILBOX0_CLR0 + 16 * cpu);
        u32 mbox_val = readl(mailbox0);
        u32 ipi = ffs(mbox_val) - 1;

        writel(1 << ipi, mailbox0);
        handle_IPI(ipi, regs);
#endif
    } else if (stat) {
        u32 hwirq = ffs(stat) - 1;

        handle_domain_irq(intc.domain, hwirq, regs);
    }
}
```

This function reads `LOCAL_IRQ_PENDING` register to figure out what interrupts are currently pending. There are 4 `LOCAL_IRQ_PENDING` registers, each corresponding to its own processor core, that's why current processor index is used to select the right one. Mailbox interrupts and all other interrupts are processed in 2 different clauses of an if statement. The interaction between different cores of a multiprocessor system is out of scope for our current discussion, so we are going to skip mailbox interrupt handling part. Now we have only the following 2 lines left unexplained.

```
        u32 hwirq = ffs(stat) - 1;

        handle_domain_irq(intc.domain, hwirq, regs);
```

This is were interrupt is passed to the next handler. First of all hardware irq number is calculated. [ffs](https://github.com/torvalds/linux/blob/v4.14/include/asm-generic/bitops/ffs.h#L13) (Find first bit) function is used to do this. After hardware irq number is calculated [handle_domain_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L622) function is called. This function uses irq domain to translate hardware irq number to Linux irq number, then checks irq configuration (it is stored in [irq_desc](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L55) struct) and calls an interrupt handler. We've seen that the handler was set to [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859). However, this handler will be overwritten by the child interrupt controller later. Now, let's examine how this happens.

### Generic interrupt controller 

We have already seen how to use device tree and `compatible` property to find the driver corresponding to some device, so I am going to skip this part and jump straight to the generic interrupt controller driver source code. You can find it in [irq-bcm2835.c](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c) file. As usual, we are going to start our exploration with the initialization function. It is called [armctrl_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L141).

```
static int __init armctrl_of_init(struct device_node *node,
				  struct device_node *parent,
				  bool is_2836)
{
	void __iomem *base;
	int irq, b, i;

	base = of_iomap(node, 0);
	if (!base)
		panic("%pOF: unable to map IC registers\n", node);

	intc.domain = irq_domain_add_linear(node, MAKE_HWIRQ(NR_BANKS, 0),
			&armctrl_ops, NULL);
	if (!intc.domain)
		panic("%pOF: unable to create IRQ domain\n", node);

	for (b = 0; b < NR_BANKS; b++) {
		intc.pending[b] = base + reg_pending[b];
		intc.enable[b] = base + reg_enable[b];
		intc.disable[b] = base + reg_disable[b];

		for (i = 0; i < bank_irqs[b]; i++) {
			irq = irq_create_mapping(intc.domain, MAKE_HWIRQ(b, i));
			BUG_ON(irq <= 0);
			irq_set_chip_and_handler(irq, &armctrl_chip,
				handle_level_irq);
			irq_set_probe(irq);
		}
	}

	if (is_2836) {
		int parent_irq = irq_of_parse_and_map(node, 0);

		if (!parent_irq) {
			panic("%pOF: unable to get parent interrupt.\n",
			      node);
		}
		irq_set_chained_handler(parent_irq, bcm2836_chained_handle_irq);
	} else {
		set_handle_irq(bcm2835_handle_irq);
	}

	return 0;
}
```

Now, let's investigate this function in more details.

```
    void __iomem *base;
    int irq, b, i;

    base = of_iomap(node, 0);
    if (!base)
        panic("%pOF: unable to map IC registers\n", node);

    intc.domain = irq_domain_add_linear(node, MAKE_HWIRQ(NR_BANKS, 0),
            &armctrl_ops, NULL);
    if (!intc.domain)
        panic("%pOF: unable to create IRQ domain\n", node);

```

The function starts with the code that reads device base address from the device three and initializes the irq domain. This part should be already familiar to you because we have seen similar code in the local irq controller driver.

```
    for (b = 0; b < NR_BANKS; b++) {
        intc.pending[b] = base + reg_pending[b];
        intc.enable[b] = base + reg_enable[b];
        intc.disable[b] = base + reg_disable[b];
```

Next, there is a loop that iterates over all irq banks. We already briefly touched irq banks in the first chapter of this lesson. The interrupt controller has 3 irq banks, which are controlled by `ENABLE_IRQS_1`, `ENABLE_IRQS_2` and `ENABLE_BASIC_IRQS` registers.  Each of the banks has its own enable, disable and pending registers. Enable and disable registers can be used to either enable or disable individual interrupts that belong to a particular bank. Pending register is used to determine what interrupts are waiting to be processed.

```
        for (i = 0; i < bank_irqs[b]; i++) {
            irq = irq_create_mapping(intc.domain, MAKE_HWIRQ(b, i));
            BUG_ON(irq <= 0);
            irq_set_chip_and_handler(irq, &armctrl_chip,
                handle_level_irq);
            irq_set_probe(irq);
        }
```

Next, there is a nested loop that is responsible for registering each supported interrupt and setting irq chip and handler.

We already saw how the same functions are used in the local interrupt controller driver. However, I would like to highlight a few important things.

* [MAKE_HWIRQ](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L57) macro is used to calculate hardware irq number. It is calculated based on bank index and irq index inside the bank.
* [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603) is a common handler that is used for interrupts of the level type. Interrupts of such type keep interrupt line set to "high" until the interrupt is acknowledged. There are also edge type interrupts that works in a different way.
* [irq_set_probe](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L667) function just unsets [IRQ_NOPROBE](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L64) interrupt flag, effectively disabling interrupt auto-probing. Interrupt auto-probing is a process that allows different drivers to discover which interrupt line their devices are connected to. This is not needed for Raspberry Pi, because this information is encoded in the device tree, however, for some devices, this might be useful. Please, refer to [this](https://github.com/torvalds/linux/blob/v4.14/include/linux/interrupt.h#L662) comment to understand how auto-probing works in the Linux kernel.

Next piece of code is different for BCM2836 and BCM2835 interrupt controllers (the first one corresponds to the RPi models 2 and 3, and the second one to RPi Model 1).  If we are dealing with BCM2836 the following code is executed.

```
        int parent_irq = irq_of_parse_and_map(node, 0);

        if (!parent_irq) {
            panic("%pOF: unable to get parent interrupt.\n",
                  node);
        }
        irq_set_chained_handler(parent_irq, bcm2836_chained_handle_irq);
```

Device tree [indicates](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75) that local interrupt controller is a parent of the global interrupt controller. Another device tree [property](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L76) tells us that global interrupt controller is connected to the interupt line number 8 of the local controller, this means that our parent irq is the one with hardware irq number 8. Those 2 properties allow Linux kernel to find out parent interrupt number (this is Linux interrupt number, not hardware number). Finally [irq_set_chained_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L636) function replaces the handler of the parent irq with [bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) function.

[bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) is very simple. Its code is listed below.

```
static void bcm2836_chained_handle_irq(struct irq_desc *desc)
{
    u32 hwirq;

    while ((hwirq = get_next_armctrl_hwirq()) != ~0)
        generic_handle_irq(irq_linear_revmap(intc.domain, hwirq));
}
```

You can think about this code as an advanced version of what we did [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L39) for the RPi OS. [get_next_armctrl_hwirq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L217) uses all 3 pending registers to figure out which interrupt was fired. [irq_linear_revmap](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L377) uses irq domain to translate hardware irq number into Linux irq number and [generic_handle_irq](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L156) just executes irq handler. Irq handler was set in the initialization function and it points to [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603) that eventually executes all irq actions associated with the interrupt (this is actually done [here](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/handle.c#L135).). For now, the list of irq actions is empty for all supported interrupts - a driver that is interested in handling some interrupt should add an action to the appropriate list. In the next chapter, we are going to see how this is done using system timer as an example.

##### Previous Page

3.2 [Interrupt handling: Low-level exception handling in Linux](../../../docs/lesson03/linux/low_level-exception_handling.md)

##### Next Page

3.4 [Interrupt handling: Timers](../../../docs/lesson03/linux/timer.md)
