## 3.3: 中断控制器

在本章中，我们将大量讨论Linux驱动程序以及它们如何处理中断。我们将从驱动程序初始化代码开始，然后看看[handle_arch_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L44)功能。

### 使用设备树查找所需的设备和驱动程序

在RPi OS中实现中断时，我们一直在使用2种设备：系统定时器和中断控制器。现在，我们的目标是了解相同设备在Linux中的工作方式。我们需要做的第一件事是找到负责使用提到的设备的驱动程序。为了找到所需的驱动程序，我们可以使用[bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dots/bcm2837-rpi-3-b.dts) 设备树文件。 这是特定于Raspberry Pi 3 Model B的顶级设备树文件，它包含其他更常见的设备树文件，这些文件在不同版本的Raspberry Pi之间共享。 如果遵循包含的链并搜索 `timer` 和 `interrupt-controller` ，则可以找到4个设备。

1. [本地中断控制器](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11)
1. [本地计时器](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L20)
1. 全局中断控制器. 它被定义在 [这里](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L109) 并修改 [这里](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L72).
1. [系统计时器](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L57)

停下来，但是为什么我们有4个设备而不是2个？这需要一些解释，我们将在下一部分中解决这个问题。

### 本地与全局中断控制器

考虑多处理器系统中的中断处理时，您应该问自己一个问题：哪个内核应负责处理特定的中断？发生中断时，是全部4个内核都中断了，还是只有一个？是否可以将特定的中断路由到特定的内核？您可能想知道的另一个问题是，如果一个处理器需要向其传递一些信息，该处理器如何通知另一个处理器？

本地中断控制器是可以帮助您回答所有这些问题的设备。它负责以下任务。

* 配置哪个内核应该接收特定的中断。
* 在内核之间发送中断。这样的中断称为`mailboxs`，并允许内核相互通信。
* 处理来自本地计时器和性能监视器中断（PMU）的中断。

[BCM2836 ARM本地外围设备](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) 手册中记录了本地中断控制器以及本地计时器的行为。

我已经多次提到本地计时器。现在您可能想知道为什么我们在系统中需要两个独立的计时器？我猜想使用本地计时器的主要用例是当您要配置所有4个内核以同时接收计时器中断时。如果使用系统定时器，则只能将中断路由到单个内核。

使用RPi OS时，我们既不使用本地中断控制器也不使用本地计时器。这是因为默认情况下，本地中断控制器的配置方式是将所有外部中断都发送到第一个内核，这正是我们所需要的。我们没有使用本地计时器，因为我们使用了系统计时器。

### 本地中断控制器

根据[bcm2837.dtsi](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75)，全局中断控制器是当地的。因此，从本地控制器开始我们的探索是有意义的。

如果我们需要找到适用于特定设备的驱动程序，则应使用[compatible](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L12) 属性。搜索该属性的值，您可以轻松地找到一个与RPi本地中断控制器兼容的驱动程序 - 这是对应的 [定义](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L315).

```
IRQCHIP_DECLARE(bcm2836_arm_irqchip_l1_intc, "brcm,bcm2836-l1-intc",
        bcm2836_arm_irqchip_l1_intc_of_init);
```

现在您可能已经猜出了驱动程序初始化的过程是什么：内核遍历设备树中的所有设备定义，并且针对每个定义，它使用 `compatible` 属性寻找匹配的驱动程序。如果找到驱动程序，则调用其初始化函数。 在设备注册过程中提供了初始化功能，在本例中，此功能是 [bcm2836_arm_irqchip_l1_intc_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L280).

```cpp
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

初始化函数采用2个参数：`node`和`parent`，它们都是类型[struct device_node](https://github.com/torvalds/linux/blob/v4.14/include/linux/of.h#L49). `node`代表设备树中的当前节点，在本例中，它指向 [这里](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L11)  `parent` 是设备树层次结构中的父节点，对于本地中断控制器，它指向 `soc` 元素（ `soc` 代表 `片上系统`，它是最简单的总线，可以直接映射所有设备寄存器到主内存。）。

节点可用于从当前设备树节点读取各种属性。例如，函数`bcm2836_arm_irqchip_l1_intc_of_init`的第一行从[reg](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837dtsi＃L13)读取设备基址属性。 但是，此过程要复杂得多，因为执行此功能时，已启用MMU，并且在我们能够访问物理内存的某个区域之前，必须将该区域映射到某个虚拟地址。 这正是[of_iomap](https://github.com/torvalds/linux/blob/v4.14/drivers/of/address.c#L759) 函数的作用: 它读取提供的节点的`reg`属性，并将由`reg`属性描述的整个内存区域映射到某个虚拟内存区域。

下一个本地计时器频率在[bcm2835_init_local_timer_frequency](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L264) 函数中初始化。 此功能没有特别说明：它仅使用某些寄存器，如[BCM2836 ARM本地外围设备](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf)手册中所述，以初始化本地计时器。

下一行需要一些解释。

```
    intc.domain = irq_domain_add_linear(node, LAST_IRQ + 1,
                        &bcm2836_arm_irqchip_intc_ops,
                        NULL);
```

Linux为每个中断分配一个唯一的整数，您可以将此数字视为唯一的中断ID。每次您想对中断执行操作时都会使用此ID（例如，分配处理程序或分配哪个CPU应该处理它）。每个中断还具有一个硬件中断号。这通常是一个数字，告诉您触发了哪个中断线。 `BCM2837 ARM外设手册` 的外设中断表位于第113页 - 您可以将此表中的索引视为硬件中断号。因此，显然，我们需要某种机制将Linux irq号映射到硬件irq号，反之亦然。如果只有一个中断控制器，则可以使用一对一的映射，但是通常情况下，需要使用更复杂的机制。在Linux中[struct irq_domain](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L152)实现了这种映射。每个中断控制器驱动程序应创建自己的`irq`域，并注册该域可以处理的所有中断。注册函数返回Linux irq号，该编号以后将用于处理中断。

接下来的6行负责向`irq`域注册每个受支持的中断。

```cpp
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

根据[BCM2836 ARM本地外设](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) 手动本地中断控制器处理10种不同的中断: 0-3是本地计时器的中断，4-7是邮箱中断，用于进程间通信，8对应于全局中断控制器生成的所有中断，中断9是性能监视器中断. [这里](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L67) 您会看到驱动程序定义了一组常量，每个常量都包含硬件irq号。上面的注册代码注册所有中断，但邮箱中断除外，邮箱中断是单独注册的。为了更好地了解注册码让我们来看看[bcm2836_arm_irqchip_register_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L154) function.


```
static void bcm2836_arm_irqchip_register_irq(int hwirq, struct irq_chip *chip)
{
    int irq = irq_create_mapping(intc.domain, hwirq);

    irq_set_percpu_devid(irq);
    irq_set_chip_and_handler(irq, chip, handle_percpu_devid_irq);
    irq_set_status_flags(irq, IRQ_NOAUTOEN);
}
```

第一行执行实际的中断注册。 [irq_create_mapping](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdomain.c#L632) 将硬件中断号作为输入并返回Linux irq号。

[irq_set_percpu_devid](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L849)将中断配置为`每个CPU`，因此只能在当前处理器上处理中央处理器。这非常有道理，因为我们现在讨论的所有中断都是本地中断，并且所有中断只能在当前CPU上处理。

[irq_set_chip_and_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L608), 顾名思义，设置irq芯片和irq处理程序。 Irq芯片是一种特殊的结构，需要由驱动程序创建，该结构具有用于屏蔽和取消屏蔽特定中断的方法。我们正在检查的驱动程序现在定义了3种不同的irq芯片: [timer](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L118) 芯片, [PMU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L134) 芯片 和 [GPU](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L148) 芯片, 它控制由外部外围设备生成的所有中断。处理程序是负责处理中断的功能。在这种情况下，处理程序设置为通用 [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859) 函数. 稍后，该处理程序将由全局中断控制器驱动程序重写。

[irq_set_status_flags](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L652) 在这种特殊情况下，设置一个标志，指示应手动启用当前中断，并且默认情况下不应启用。

回到`bcm2836_arm_irqchip_l1_intc_of_init`函数，只剩下两个调用。第一个是 [bcm2836_arm_irqchip_smp_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L243). 在此启用了邮箱中断，从而允许处理器内核相互通信。

最后一个函数调用非常重要-这是将低级异常处理代码连接到驱动程序的地方。

```
    set_handle_irq(bcm2836_arm_irqchip_handle_irq);
```

[set_handle_irq](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/irq.c#L46) 是在特定于体系结构的代码中定义的，我们已经遇到了此功能。 从上面的行中我们可以了解到[bcm2836_arm_irqchip_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2836.c#L164) 将由低级异常代码调用。函数本身在下面列出。

```cpp
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

该函数读取 `LOCAL_IRQ_PENDING` 寄存器，以找出当前正在处理的中断。有4个 `LOCAL_IRQ_PENDING` 寄存器，每个寄存器对应于其自己的处理器内核，这就是为什么使用当前处理器索引来选择正确的寄存器的原因。邮箱中断和所有其他中断在if语句的2个不同子句中处理。多处理器系统的不同内核之间的交互超出了我们当前的讨论范围，因此我们将跳过邮箱中断处理部分。现在，仅剩下以下两行无法解释。

```
        u32 hwirq = ffs(stat) - 1;

        handle_domain_irq(intc.domain, hwirq, regs);
```

这是将中断传递给下一个处理程序的地方。首先计算硬件irq数。 [ffs](https://github.com/torvalds/linux/blob/v4.14/include/asm-generic/bitops/ffs.h#L13) (查找第一位) 函数用于执行此操作. 计算出硬件irq数后 [handle_domain_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/irqdesc.c#L622) 函数被调用. 此功能使用irq域将硬件irq号码转换为Linux irq号码，然后检查irq配置 (它存储在 [irq_desc](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L55) 结构) 并调用一个中断处理程序。 我们已经看到处理程序设置为 [handle_percpu_devid_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L859). 但是，此处理程序稍后将被子中断控制器覆盖。现在，让我们检查一下这是如何发生的。

### 通用中断控制器

我们已经看到了如何使用设备树和`compatible`属性来查找与某个设备相对应的驱动程序，因此我将跳过这一部分，直接跳转到通用中断控制器驱动程序源代码。 你可以在找到它在 [irq-bcm2835.c](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c) 文件. 和往常一样，我们将从初始化功能开始探索。 叫做 [armctrl_of_init](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L141).

```cpp
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

现在，让我们更详细地研究此功能。

```cpp
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

该函数以从设备3读取设备基地址并初始化irq域的代码开始。您应该已经熟悉此部分，因为我们已经在本地irq控制器驱动程序中看到了类似的代码。

```cpp
    for (b = 0; b < NR_BANKS; b++) {
        intc.pending[b] = base + reg_pending[b];
        intc.enable[b] = base + reg_enable[b];
        intc.disable[b] = base + reg_disable[b];
```

接下来，有一个循环遍历所有irq库。在本课程的第一章中，我们已经简短地谈到了irq银行。中断控制器具有3个irq bank，由 `ENABLE_IRQS_1` ，`ENABLE_IRQS_2` 和 `ENABLE_BASIC_IRQS` 寄存器控制。每个存储区都有其自己的启用，禁用和挂起寄存器。启用和禁用寄存器可用于启用或禁用属于特定存储区的单个中断。待处理寄存器用于确定正在等待处理的中断。

```cpp
        for (i = 0; i < bank_irqs[b]; i++) {
            irq = irq_create_mapping(intc.domain, MAKE_HWIRQ(b, i));
            BUG_ON(irq <= 0);
            irq_set_chip_and_handler(irq, &armctrl_chip,
                handle_level_irq);
            irq_set_probe(irq);
        }
```

接下来，有一个嵌套循环，负责注册每个受支持的中断并设置irq芯片和处理程序。

我们已经看到了本地中断控制器驱动程序中如何使用相同的功能。但是，我想强调一些重要的事情。

* [MAKE_HWIRQ](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L57) 宏用于计算硬件irq号。它是根据银行内部的银行指数和irq指数计算的。
* [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603) 是用于级别类型的中断的通用处理程序。此类中断将中断线设置为“高”，直到确认该中断为止。还有边缘类型中断以不同的方式工作。
* [irq_set_probe](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L667) 函数只是未设置 [IRQ_NOPROBE](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L64) 中断标志, 有效地禁用中断自动探测。中断自动探测是允许不同的驱动程序发现其设备连接到哪条中断线的过程. Raspberry Pi不需要此功能，因为此信息被编码在设备树中，但是，对于某些设备，这可能很有用。请参阅 [这个](https://github.com/torvalds/linux/blob/v4.14/include/linux/interrupt.h#L662) 评论以了解自动探测如何在Linux内核中工作。

`BCM2836`和`BCM2835`中断控制器的下一段代码是不同的（第一个对应于RPi模型2和3，第二个对应于RPi模型1）。如果我们正在处理BCM2836，则执行以下代码。

```cpp
        int parent_irq = irq_of_parse_and_map(node, 0);

        if (!parent_irq) {
            panic("%pOF: unable to get parent interrupt.\n",
                  node);
        }
        irq_set_chained_handler(parent_irq, bcm2836_chained_handle_irq);
```

设备树[表明](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L75)本地中断控制器是全局中断控制器的父级。另一个设备树 [属性](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L76) 告诉我们全局中断控制器已连接到本地控制器的中断线8，这意味着您的父irq是硬件irq 8。 这2个属性允许Linux内核找出父中断号（这是Linux中断号，而不是硬件号）。最后[irq_set_chained_handler](https://github.com/torvalds/linux/blob/v4.14/include/linux/irq.h#L636) 函数将父irq的处理程序替换为 [bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) 功能.

[bcm2836_chained_handle_irq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L246) 很简单,其代码在下面列出。

```cpp
static void bcm2836_chained_handle_irq(struct irq_desc *desc)
{
    u32 hwirq;

    while ((hwirq = get_next_armctrl_hwirq()) != ~0)
        generic_handle_irq(irq_linear_revmap(intc.domain, hwirq));
}
```

您可以将这段代码视为我们所做工作的高级版本 [这里](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson03/src/irq.c#L39) 用于RPi OS. [get_next_armctrl_hwirq](https://github.com/torvalds/linux/blob/v4.14/drivers/irqchip/irq-bcm2835.c#L217) 使用所有3个暂挂寄存器来确定触发了哪个中断。 [irq_linear_revmap](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdomain.h#L377) 使用irq域将硬件irq号码转换为Linux irq号码和 [generic_handle_irq](https://github.com/torvalds/linux/blob/v4.14/include/linux/irqdesc.h#L156) 只是执行irq处理程序。在初始化函数中设置了Irq处理程序 它指向 [handle_level_irq](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/chip.c#L603) 最终执行与中断相关的所有irq动作 (这实际上是完成的 [这里](https://github.com/torvalds/linux/blob/v4.14/kernel/irq/handle.c#L135).). 目前，所有支持的中断的irq操作列表为空 - 对处理某些中断感兴趣的驱动程序应在相应的列表中添加一个操作。在下一章中，我们将以系统计时器为例来了解如何完成此操作。

##### 上一页

3.2 [中断处理：Linux中的低级异常处理](../../../docs/lesson03/linux/low_level-exception_handling.md)

##### 下一页

3.4 [中断处理：计时器](../../../docs/lesson03/linux/timer.md)
