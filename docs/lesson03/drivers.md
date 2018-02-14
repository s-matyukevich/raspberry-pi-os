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
