## 1.4：Linux启动顺序

### 搜索入口点

看完`Linux`项目结构并研究了如何构建之后, 下一个逻辑步骤就是找到程序入口点. 对于许多程序而言, 此步骤可能是微不足道的, 但对于Linux内核而言却并非如此. 

我们要做的第一件事是看一下[arm64链接描述文件](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S). 我们已经看到了链接脚本 [如何在主makefile中使用](https://github.com/torvalds/linux/blob/v4.14/Makefile#L970).  从这一行, 我们可以轻松推断出可以找到特定体系结构的链接脚本的位置. 

应该提到的是, 我们将要检查的文件不是实际的链接描述文件, 而是一个模板, 通过使用一些模板的实际值替换一些宏, 可以从中构建实际的链接描述文件. 但是正是因为该文件主要由宏组成, 所以读取和在不同体系结构之间移植变得更加容易. 

我们在链接描述文件中可以找到的第一部分称为 [.head.text](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S#L96). 这对我们非常重要, 因为应该在本节中定义入口点. 如果您仔细考虑一下, 那么它是很完美的意思：在加载内核之后, 将二进制映像的内容复制到某个内存区域, 然后从该区域的开头开始执行. 这意味着仅仅通过搜索谁使用`.head.text`节, 我们就可以找到入口点. 实际上, `arm64` 架构只有一个文件 [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S), 该文件会使用 [__HEAD](https://github.com/torvalds/linux/blob/v4.14/include/linux/init.h#L90) 宏, `_HEAD` 宏在编译前的预处理阶段会被替换为 `.section ".head.text","ax"`. 

我们可以在`head.S`文件中找到的第一行可执行文件是 [这里](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L85). 这里使用 arm 的 `b(branch)` 指令指示处理器跳转到 [stext](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L116) 函数. 这是启动内核后执行的第一个函数. 

逻辑上, 下一步我们应该继续探索 `stext` 函数内部正在发生的事情-但是我们还没有准备好. 首先, 我们必须在`RPi OS`中实现类似的功能, 这将在接下来的几课中介绍. 我们现在要做的是研究一些与内核引导有关的关键概念. 

### Linux引导程序和引导协议

当linux内核启动时, 它假定机器硬件已准备成某种 `已知状态`. 定义此状态的规则集称为`启动协议`, 对于 `arm64` 体系结构, 此文档记录在[这里](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt). 例如, 它定义了执行只能在主CPU上开始, 必须关闭内存映射单元, 并且必须禁用所有中断. 

但是由谁负责使计算机进入已知状态？通常, 有一个特殊的程序在内核之前运行并执行所有初始化. 这个程序叫做引导程序(`bootloader`). `Bootloader` 的代码是与计算机硬件相匹配的特定代码, `Raspberry PI`也有自己特定的 `bootloader` 代码. Raspberry PI 的引导程序内置在主板中. 我们只能使用 [config.txt](https://www.raspberrypi.org/documentation/configuration/config-txt/) 文件来自定义其行为. 

### UEFI引导

除了 `Bootloader` 外, 还可以在内核映像中内置一个引导加载程序. 此引导加载程序只能在支持 [Unified Extensible Firmware Interface(UEFI)](https://en.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface) 的平台上使用. 支持UEFI的设备为正在运行的软件提供了一组标准化服务, 这些服务可用于找出有关机器本身及其功能的所有必要信息. UEFI 还要求计算机固件应能够以 [Portable Executable(PE)](https://en.wikipedia.org/wiki/Portable_Executable) 格式运行可执行文件. Linux 内核的 UEFI 引导加载程序就利用了此功能：它在 Linux 内核映像的开头注入 `PE` 标准头部格式信息, 以便计算机固件认为该映像是普通的 `PE` 文件. [efi-header.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S) 文件中就完成了这个操作. 该文件定义了 [__EFI_PE_HEADER](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L13) 宏, 该宏在 [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L98) 内部使用. 

在 `__EFI_PE_HEADER` 内部定义的一项重要属性是用于说明 [UEFI入口点](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L33) 的属性和入口点本身(可以在 [efi-entry.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-entry.S#L32) 中找到). 从此位置开始, 您可以遵循源代码并检查 UEFI 引导加载程序到底在做什么(源代码本身或多或少简单明了). 但是我们将在此处停止, 因为本节的目的不是详细检查 UEFI 引导程序, 而是让您大致了解 UEFI 是什么以及 Linux 内核如何使用它. 

### 设备树

当我开始研究 Linux 内核的启动代码时, 发现了很多关于设备树(`device tree`) 的内容. 它似乎是一个必不可少的概念, 我认为有必要对其进行讨论. 

当我们在开发 `Raspberry PI OS` 内核时, 我们从 [BCM2837 ARM Peripherals手册](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf) 中找出了一个特定的内存映射寄存器在物理内存上的确切偏移量. 显然每个主板的偏移量信息都是不同的, 我们很幸运, 我们只需要支持 BCM2873 这一个. 但是, 如果我们需要支持数百个不同的主板怎么办？如果我们尝试在内核代码中硬编码每个主板的信息, 那将是一团糟. 即使我们设法做到这一点, 我们又如何确定当前使用的是哪种主板？例如, `BCM2837` 没有提供任何将此类信息传递给正在运行的内核的方法. 

设备树为我们提供了解决上述问题的方法. 设备树一种特殊格式, 可用于描述计算机硬件. 可以在[device tree org](https://www.devicetree.org/)中找到设备树规范. 在执行内核之前, 引导程序会选择适当的设备树文件, 并将其作为参数传递给内核. 如果您查看 `Raspberry PI` SD 卡的引导分区(boot分区)中的文件, 会找到很多 `.dtb` 文件. `.dtb`是编译之后的设备树文件. 可以在 `config.txt` 中配置来启用或禁用 `Raspberry PI` 的硬件. [Raspberry PI官方文档](https://www.raspberrypi.org/documentation/configuration/device-tree.md) 中更详细地描述了此过程. 

好的, 现在该看一下实际的设备树的文件了. 作为快速练习, 让我们尝试为[Raspberry PI 3 Model B](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/)查找设备树. 从 [文档](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2837/README.md) 中, 我们可以得出结论, `Raspberry PI 3 Model B` 使用的芯片名为 `BCM2837`. 如果你搜索此名称, 则可以找到 [/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts) 文件. 如您所见, 它只是包含了`arm` 体系下的相同文件. 这很合理, 因为 ARM.v8 处理器也支持32位模式. 

接下来, 我们可以找到 [bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts) 属于 [arm](https://github.com/torvalds/linux/tree/v4.14/arch/arm) 架构. 我们已经看到设备树文件可以包含在另一个文件中.  `bcm2837-rpi-3-b.dts` 就是这种情况 - 它仅包含特定于 `BCM2837`的那些定义, 并重用其他所有内容. 例如, `bcm2837-rpi-3-b.dts` 指定[设备应该具有 1GB 的内存](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L18). 

正如我之前提到的, `BCM2837` 和 `BCM2835` 具有相同的外围硬件, 并且, 如果追寻包含链, 您可以找到 [bcm283x.dtsi](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi) 实际上定义了大多数此类硬件. 

设备树由彼此嵌套的块组成. 在顶层, 我们通常可以找到诸如 [cpus](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L30) 或 [memory](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L17), 块的含义通过块的节点名称也能简明的看出. 我们可以在 `bcm283x.dtsi` 中找到的另一个有趣的顶级节点是 [SoC](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/boot/dts/bcm283x.dtsi#L52) 的意思是 [System on chip](https://en.wikipedia.org/wiki/System_on_a_chip) . 它告诉我们所有外围设备都通过*内存映射寄存器*直接映射到某个内存区域. `soc` 节点用作所有外围设备的父节点. 它的子节点之一是 [gpio](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L147) 节点. 此节点定义 [reg = <0x7e200000 0xb4>](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L149) 属性, 该属性告诉我们 GPIO 的内存映射寄存器位于 `0x7e200000 ~ 0x7e2000b4` 区域. gpio 节点的子节点之一具有[以下定义](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L351)

```
uart1_gpio14：uart1_gpio14 {
        brcm, pins = <14 15>;
        brcm, function = <BCM2835_FSEL_ALT5>;
};
```

这个定义告诉我们, 如果为引脚14和15选择了替代功能5, 则这些引脚将连接到 `uart1` 设备. 你可以很容易地猜出 `uart1` 设备就是我们已经使用过的`Mini UART`. 

你需要了解的有关设备树的另外重要的一件事是：设备树格式是可扩展的. 每个设备可以定义自己的属性和嵌套块. 这些属性透明地传递给设备驱动程序, 解释它们是驱动程序的职责. 但是内核如何找出设备树中的块与正确的驱动程序之间的对应关系？内核使用 `compatible` 属性来做到这一点. 例如, 对于 `uart1` 设备, `compatible` 属性是这样指定的

```
compatible = "brcm,bcm2835-aux-uart";
```

实际上, 如果您在Linux源代码中搜索 `bcm2835-aux-uart`, 则可以找到匹配的驱动程序, 该驱动程序定义在 [8250_bcm2835aux.c](https://github.com/torvalds/linux/blob/v4.14/drivers/tty/serial/8250/8250_bcm2835aux.c)

### 结论

你可以考虑将本章作为阅读 `arm64` 引导代码的准备 - 在没有了解我们刚刚探讨的概念的情况下, 你将很难学习它. 

在下一课中, 我们将返回到 `stext` 函数, 并详细研究其工作方式. 

#### 上一页

1.3 [内核初始化：内核构建系统](./build-system.md)

#### 下一页

1.5 [内核初始化：练习](../exercises.md)
