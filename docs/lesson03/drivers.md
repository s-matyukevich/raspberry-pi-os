## Lesson 3: How linux drivers handles interrupts.

In this lesson we are going to talk a lot about linux drivers and how they handles interrupts. We will start with driver initialization code and then take a look on how interrupts are processed after [handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44) function.

### Using device tree to find out needed devices and drivers.

When implementing RPi OS we have deen working with 2 devices: system timer and interrupt controller. Now our goal will be to figure out how the same devices works in linux. The first thing we need to do is to find drivers that are responsible for working with those devices. And in order to find needed drivers we can use a[bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts) device tree file. This is the top level device tree file that is specific for Raspberry Pi 3 Model B, it includes other more common device tree files, that are shared between differen versions of Raspberry Pi. If you follow the chain of includes and search for `timer` and `interrupt-controller` you can find 4 devices.

1. [Local interrupt controller](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11)
1. [Local timer](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L20)
1. Global interrupt controller. It is defined [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L109) and modified [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L72)
1. [System timer](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57)

Stop, but why do we have 4 devices instaed of 2? This requires some explanation.

### Local vs global interrupt controllers.

When you think about interrupt handling in multiprocessor systerms, one question you shoud ask yourself is which core should be responsible for processing a particular interrupt? When an interrupt occures, are all 4 cores interrupted? Or only a single one? Is it posible to route a particular interrupt to a particular core? Another question you may wonder is how one processor can notify another processor if he needs to pass some information to it? 

Local interrupt controller is a device that can help you in answering all those question. It is responsible for the following tasks.

* Configuring which core should receive specific interrupt.
* Sending interrupts between cores. Such interrupts are called "mailboxes" and allows cores to communicate one with each other. 
* Handling interrupts from local timer and performance monitors interrupts (PMU).

The behaviour of local interrupt controller as well as local timer is documented in [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) manual.

I already mentioned local timer several times. Now you probably wonder why do we need two separate independet timers in the system? I guess that the main usecase for using local timer is when you want to configure all 4 cores to receive timer interrupts simulteneasly. If you use system timer you can only route interrupts to a simngle core. 

When working with RPi OS we didn't work with either local interrupt controller or local timer. That is because by default local interrupt controller is configured in such a way that all external interrupts are send to the first core, with is exactly what we need. We didn't use local timer because we used system timer instead.

### Local interrupt controller driver

Accordingly to the [bcm2837.dtsi](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75) global interrutp controller is a child of the local one. Thus it make sense to start our exploration with the local controller.

If we need to find a driver that works with some device, we should use [compatible](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L12). Searching for the value of this property you can easily find that there is a single driver that is compatible with RPi local interrupt controller - here is the coresponsig [definition] (https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L315). 

```
IRQCHIP_DECLARE(bcm2836_arm_irqchip_l1_intc, "brcm,bcm2836-l1-intc",
		bcm2836_arm_irqchip_l1_intc_of_init);
```

From this defenition you can infer what is ther procedure of driver initialization: the kernrl walks though all device dfenitions in the device tree and for each defenition it looks for a matching driver using "compatible" property. If driver is found then its initialization function is called. Initialization function is provided during device registration, and in our case this funciton is [bcm2836_arm_irqchip_l1_intc_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L280) 

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

Initialization function takes 2 parameters: 'node' and 'parent', both of them are of the type [struct device_node](https://github.com/torvalds/linux/blob/v4.14/include/linux/of.h#L49). `node` represent curent node in the device tree, and in our case it points [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11) . `parent` is a parent node in the device tree hierarchy, and in our case it points to `soc` element (`soc` stands for "system on chip" and it is the simplest posible bus which maps all device registers directly to main memory)

`node` can be used to read varios properties from the current device tree node. For example, the first line of the `bcm2836_arm_irqchip_l1_intc_of_init` function reads device base address from [reg](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L13) property. However, the process is more complicated then that, because when this function is executed MMU is already enabled, and before we will be able to access some region of physical memory we must map this region to some virtula address. This is exactly what [of_iomap](https://github.com/torvalds/linux/blob/v4.14/drivers/of/address.c#L759) function is doing: it reads `reg` property of the provided node and maps the whole memory region, described by `reg` property, to some virual memory region.

Next local timer frequency is initialized in ][bcm2835_init_local_timer_frequency](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L264) function. There is nothing specific about this funciton: it just uses some of the registers, described in [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) manual, to initialize local timer.

Next line require some explanations.

```
	intc.domain = irq_domain_add_linear(node, LAST_IRQ + 1,
					    &bcm2836_arm_irqchip_intc_ops,
					    NULL);
```

Linux assinges a unique integer number to each interrupt, you can think about this number as a unique interrupt ID. This ID is used each time you want to do somthing with an interrupt (for example, assign a handler, or assign wich CPU should handle it). Each interrupt also has a hardware interrupt handler. This is usually a number that tells which interrupt line was triggered. `BCM2835 ARM Peripherals manual` has a peripherial interrupt table at page 113 - you can think about index in this table as a hardware interrupt number. So obviously we need some mechnizm to map linux irq numbers to hardware irq number and vise versa. If there is only one interrupt controller it would be posible to use one to one mapping, but in general case a more sophisticated mechnizm need to be used. In linux [struct irq_domain](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L152) implements such mechanizm. Eahc interrupt controller driver should create its own irq domain and register allinterrupts that it can handle with this domain. Registration function returnes linux irql number that later is used to work with interrupt.

Next 6 lines are responsible for registering each supported interrupt with irq domain.

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

Accordingly to [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) manual local interrupt controller handles 10 different interrupts: 0 - 3 are interrupts from local timer, 4 - 7 are mailbox interraupts, which are used in interprocess communication, 8 correspons to all interrupts generated by the global interrupt controller and interrupt 9 is a prefomance monitor interrupt. [Here](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L67) you can see that the driver defines a constant that holds hardware irq number per each interrupt. The registration code above registeres all interrupts, except mailbox interrupts, wich are registered separately. In order to understand the registration code better lets examine [bcm2836_arm_irqchip_register_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L154) function.

```
static void bcm2836_arm_irqchip_register_irq(int hwirq, struct irq_chip *chip)
{
	int irq = irq_create_mapping(intc.domain, hwirq);

	irq_set_percpu_devid(irq);
	irq_set_chip_and_handler(irq, chip, handle_percpu_devid_irq);
	irq_set_status_flags(irq, IRQ_NOAUTOEN);
}
``` 

The first line here perfomes actuall interrupt registration. [irq_create_mapping](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdomain.c#L632) takes hardware interrupt number as an input and returned linux interrupt number. 

[irq_set_percpu_devid](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L849) configures interrupt as "per CPU", so that  it will be handled only on the current CPU. This make perfect sense, because all interrupts that we are discussing now are local and they all can be handled only on current CPU.

[irq_set_chip_and_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L608), as it name suggest, sets irq chip and irq handler. Irq chip is a special structure, wich need to be created by the driver, which has methods for masking and unmasing particular interrupt. The driver that we are examining right now defines 3 different irq chips: [timer](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L118) chip, [PMU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L134) chip and [GPU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L148), which controlls all interrupts generated by external pheripherial devices. Handler is a function that is resonsible for processing an interrupt. In this case handler is set to generic [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859) function that passes that execution to child interrupt controller (more on this later). 

[irq_set_chip_and_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L652) in this particular case sets a flag, indication that current interrupt should not be enabled manually and not enabled by default.

Going back to the `handle_percpu_devid_irq` function, there are only 2 function calls left. The first one is [bcm2836_arm_irqchip_smp_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L243). Here lbox interrupts arae anabled allowing processor cores to communicate with each other. 

The last function call is extrmely important - this is the place were low level exception handling code is connected to the driver.

```
	set_handle_irq(bcm2836_arm_irqchip_handle_irq);
``` 

[set_handle_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L46) is defined in architecture specific code and we already discussed this function. From the line above we can understand thst [bcm2836_arm_irqchip_handle_irq]()https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L164 will be called by the low level exception code after it finishes all preparation work. The function itself is listed below.

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

This function reads `LOCAL_IRQ_PENDING` register to  figure out what interrupts are currently pending. There are 4 `LOCAL_IRQ_PENDING` registers, each coresponding to its own processor core, thats why current processor index is used to select the right one. Mailbox interrupts and all other interrupts are processed in 2 different clauses of an if statement. Interraction between different cores of multiprocessor system is out of scope for our current discusstion, so we are going to skip mailbox interrupt handling part. Now we have only the following 2 lines left uneplained 

```
		u32 hwirq = ffs(stat) - 1;

		handle_domain_irq(intc.domain, hwirq, regs);
```

This is were interrupt is passed to the next handler. First of all hardware irq number is calculated. [ffs](https://github.com/torvalds/linux/blob/v4.14/include/asm-generic/bitops/ffs.h#L13) (Find first bit) function is used to do this. After hardware irq number is calculated [handle_domain_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L622) function is called. This function uses  irq domain to translate hardware irq number to linur irq number, then checks irq configuration (it is stored in [irq_desc](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L55) struct) and calls previously registered interrupt handler. In our case this handler is [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859). You can probably guess that at this point work should be somehow passed to the  global interrupt controller, but examining  source code of `handle_percpu_devid_irq` gives us no clues. This handler passes execution to the [action](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L63) field of `irq_desc` struct. But were the `action` field is set? You might guess that driver coresponding to global irq controller should somehow do this. And that is something we are going to explore in the next section.

### Generic interrupt controller 

We have already seen how to use device tree and `compatible` property to  find the driver coresponding to some device, so I am going to skip this part and jump straight to the driver source code. You can find it in [irq-bcm2835.c](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c) file. As usual, we are going to start our exploration with the initialization function. It is called [armctrl_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L141). It starts with the code that reads device ase address from the device three and initialize irq domain. 

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

This part should be already familiar to you, because we have seen similar code in the local irq controller driver.

Next, there is a loop that iterates over all irq banks. We alredy berfelly touched irq banks in the first chapter of this lesson. The interrupt controller has 3 irq banks, witch are controlled by `ENABLE_IRQS_1`, `ENABLE_IRQS_2` and `ENABLE_BASIC_IRQS` registers.  Each of the banks has its own enable, disable and pending registers. Enable and disable registers can be used to either enable or disable individual interrupts that belongs to a particular bank. Pending register is used to determing what interrupts are wating to be processed. Here is the code that initalized those 3 registers for each of the banks.

```
	for (b = 0; b < NR_BANKS; b++) {
		intc.pending[b] = base + reg_pending[b];
		intc.enable[b] = base + reg_enable[b];
		intc.disable[b] = base + reg_disable[b];
```

There is also a nesterd loop that is responsible for registering each supported interrupt and setting irq chip and handler.

```
		for (i = 0; i < bank_irqs[b]; i++) {
			irq = irq_create_mapping(intc.domain, MAKE_HWIRQ(b, i));
			BUG_ON(irq <= 0);
			irq_set_chip_and_handler(irq, &armctrl_chip,
				handle_level_irq);
			irq_set_probe(irq);
		}
```

We already saw how the same functions are used  in local interrupt controller driver. However, I would like to highlight a few important things.

* [MAKE_HWIRQ](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L57) makro is used to calculate hardware irq number. Iti is calculated based on bank index and irq index inside the bank.
* [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603) is a common handler that is used for interrupts of the level type. Interrupts of such type keeps interrupt line set to "high" untill interrupt is acknoleged. There is also edge type interrupt that works in a different way.
* [irq_set_probe](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L667) function just unsets [IRQ_NOPROBE](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L64) interrupt flag, effectively enabling interrupt autoprobing. Interrupt autoprobing is a process that allows different devices to discover wich interrupt line they are connected to. This is not needed for Parspberry Pi, becuse this information is encoded in device tree, however for some devices this might be usefull. Please, refer to [this](https://github.com/torvalds/linux/blob/v4.14/include/linux/interrupt.h#L662) comment to understand how autoprobing works in linux kernel.

Next pice of code is different for BCM2836 and BCM2835 interrupt controllers (the first one coresponds to RPi Model 2 and 3, and second one to RPi Model 1).  If we are dealing with BCM2836 the following code is executed.

```
		int parent_irq = irq_of_parse_and_map(node, 0);

		if (!parent_irq) {
			panic("%pOF: unable to get parent interrupt.\n",
			      node);
		}
		irq_set_chained_handler(parent_irq, bcm2836_chained_handle_irq);
```

Device tree [indicates](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75) that local interrupt controller is a parent of global interrupt controller. Another device tree [property](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L76) tells us that global interrupt controller is connected to interupt line number 8 of the local controller, this means that our parent irq is the one with hardware irq number 8. Those 2 properties allow linux kernel to find out parent interrupt number (this is linux interrupt number, not hardware number). Finally [irq_set_chained_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L636) function replaces the handler of the parent irq with [bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) function.

[bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) is very simple. Its code is listed below.

```
static void bcm2836_chained_handle_irq(struct irq_desc *desc)
{
	u32 hwirq;

	while ((hwirq = get_next_armctrl_hwirq()) != ~0)
		generic_handle_irq(irq_linear_revmap(intc.domain, hwirq));
}
```

You can think about this code as an advanced version os what we did [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L39) for RPi OS.[get_next_armctrl_hwirq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L217) uses all 3 pending registers to figure out which interrupt was fired. [irq_linear_revmap](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L377) uses irq domain to translate hardware irq number into linux irq number and [generic_handle_irq](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L156) just executes irq handler. Irq handler was set in the initialization function and it points to [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603) that eventually executes all irq actions assotiated with interrupt (this is actually done [here](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/handle.c#L135)). For now the list of irq actions is empty for all suported interrupts - a driver that is interested in handling some interrupt should add an action to the appropriate list. Next we are goin to see how this is done using system timer as an example.

### System timer
