## 1.4: Linux startup sequence

### Searching for an entry point

After taking a look at the Linux project structure and examining how it can be built, next logical step is to find the program entry point. This step might be trivial for a lot of programs, but not for the Linux kernel. 

The first file we are going to do is to take a look at [arm64 linker script](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S). We have already seen how the linker script [is used in the main makefile](https://github.com/torvalds/linux/blob/v4.14/Makefile#L970). From this line, we can easily infer, where the linker script for a particular architecture can be found.

It should be mentioned that the file we are going to examine is not an actual linker script - it is a template, from which the actual linker script is built by substituting some macros with their actual values. But precisely because this file consists mostly of macros (similar to [this one](https://github.com/torvalds/linux/blob/v4.14/include/asm-generic/vmlinux.lds.h#L514)) it becomes much easier to read and to port between different architectures.

The first section that we can find in the linker script is called [.head.text](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S#L96) This is very important for us because the entry point should be defined in this section. If you think a little about it, it makes a perfect sense: after the kernel is loaded, the content of the binary image is copied into some memory area and execution is started from the beginning of that area. This means that just by searching what files uses `.head.text` section we should be able to find the entry point. And indeed, `arm64` architecture has a single file the uses [__HEAD](https://github.com/torvalds/linux/blob/v4.14/include/linux/init.h#L90) macro, which is expanded to `.section    ".head.text","ax"` - this file is [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S)

The first executable line, that we can find in `head.S` file is [this one](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L85). Here we use arm assembler `b` of `branch` instruction to jump to the [stext](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L116) function. And this is the first function that is executed after you boot the kernel.

Next logical step is to explore what is going on inside the `stext` function - but we are not ready yet. First, we have to implement similar functionality in the RPi OS, and that is something we will cover in the next few lessons. What we are going to do right now is to examine a few critical concepts, related to kernel boot.

### Linux bootloader and boot protocol

When linux kernel boots it assumes that the machine hardware is prepared in some "known state". The set of rules that defines this state is called "boot protocol", and for `arm64` architecture it is documented [here](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt). Among other things, it defines, for example, that the execution must start only on primary CPU, Memory Mapping Unit must be turned off and all interrupts must be disabled. 

Ok, but who is responsible for bringing a machine into that known state? Usually, there is a special program that runs before the kernel and performs all initializations. This program is called `bootloader`. Bootloader code may be very machine specific, and this is the case, with Raspberry PI. Raspberry PI has a bootloader that is is built into the board. We can only use [config.txt](https://www.raspberrypi.org/documentation/configuration/config-txt/) file to customize its behavior. 

### UEFI boot

However, there is one boot loader that can be built into the kernel image itself. This bootloader can be used only on the platforms that support [Unified Extensible Firmware Interface (UEFI)](https://en.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface). Devices that support UEFI provide a set of standardized services to the running software and those services can be used to figure out all necessary information about the device itself and its capabilities. UEFI also requires that computer firmware should be capable of running executable files in [Portable Executable (PE)](https://en.wikipedia.org/wiki/Portable_Executable) format. Linux kernel UEFI bootloader makes use of this feature: it injects `PE` header at the beginning of the Linux kernel image so that computer firmware think that the image is a normal `PE` file. This is done in [efi-header.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S) file. This file defines [__EFI_PE_HEADER](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L13) macro, which is used inside [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L98).

One important header that is defined inside `__EFI_PE_HEADER` is the one that tells about the [location of the UEFI entry point](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L33) and the entry point itself can be found in [efi-entry.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-entry.S#L32). Starting from this location, you can follow the source code and examine what exactly UEFI bootloader is doing (the source code itself is more or less straightforward). But we are going to stop here because the purpose of this section is not to examine UEFI bootloader in details, but instead, give you a general idea what UEFI is and how Linux kernel uses it.

### Device Tree

When I started to examine the startup code of the Linux kernel, I found a lot of mentions of so-called `Device Trees`. It appears to be an essential concept, and I consider it necessary to discuss it.

When we were working on `Raspberry PI OS` kernel, we used [BCM2835 ARM Peripherals manual](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf) to figure out what is the exact offset at which a particular memory mapped register is located. This information obviously is different for each board, and we are lucky that we have to support only one of them. But what if we need to support hundreds of different boards? It would be a total mess if we try to hardcode information about each board in the kernel code. And even if we manage to do so, how would we figure out what board we are using right now? `BCM2835`, for example, doesn't provide any means of communicating such information to the running kernel.

Device tree provides us with the solution to the problem, mentioned above. It is a special format that can be used to describe computer hardware. Device tree specification can be found [here](https://www.devicetree.org/). Before the kernel is executed, bootloader selects proper device tree file and passes it as an argument to the kernel. If you take a look at the files in the boot partition on a Raspberry PI SD card, you can find a lot of `.dtb` files here. `.dtb` files are compiled device tree files. You can select some of them in the `config.txt` to enable or disable some Raspberry PI  hardware device. This process is described in more details in the [Raspberry PI official documentation](https://www.raspberrypi.org/documentation/configuration/device-tree.md)

Ok, now it is time to take a look at how an actual device tree looks like. As a quick exercise, let's try to find a device tree for [Raspberry PI 3 Model b](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/). From the [documentation](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2837/README.md) we can figure out that `Raspberry PI 3 Model b` uses a chip that is called `BCM2837`. If you search for this name you can find [/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts)  file. As you might see it just includes the same file from `arm` architecture. This makes a perfect sense because `ARM.v8` processor supports 32-bit mode as well. 

Next, we can find [bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts) belonging to the [arm](https://github.com/torvalds/linux/tree/v4.14/arch/arm) architecture. We already saw that device tree files could include on another. This is the case with the  `bcm2837-rpi-3-b.dts` - it only contains those definitions, that are specific for `BCM2837` and reuses everything else. For example,  `bcm2837-rpi-3-b.dts` specifies that [the device now have 1GB of memory](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L18). 

As I mentioned previously, `BCM2837` and `BCM2835` have an identical peripheral hardware, and, if you follow the chain of includes, you can find [bcm283x.dtsi](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi) that actually defines most of this hardware. 

A device tree definition consists of the blocks nested one in another. At the top level we usually can find such block as [cpus](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L30) or [memory](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L17) The meaning of those blocks should be quite self-explanatory. Another interesting top-level element that we can find in the `bcm283x.dtsi` is [sos](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L52) that means [System on a chip](https://en.wikipedia.org/wiki/System_on_a_chip) It tells us that all peripheral devices are directly mapped to some memory area via memory mapped registers. `soc` element serves as a parent element for all peripheral devices. One of its children is [gpio](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L147) element. This element defines [reg = <0x7e200000 0xb4>](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L147) property that tells us that GPIO memory mapped registers are located in the  `[0x7e200000 : 0x7e2000b4]` region. One of the childern of `gpio` element has the [following definition](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L474)

```
uart1_gpio14: uart1_gpio14 {
        brcm,pins = <14 15>;
        brcm,function = <BCM2835_FSEL_ALT5>;
};
``` 
This definition tells us that if alternative function 5 is selected for pins 14 and 15 those pins will be connection to `uart1` device. You can easily gues that `uart1` device is the Mini UART that we have used already.

One important thing that you need to know about device trees is that the format is extendable. Each device can define its own properties and nested blocks. Those properties are transparently passed to the device driver, and it is driver responsibility to interpret them. But how can the kernel figure out the correspondence between a block in a device tree and the right driver? It uses `compatible` property to do this. For example, for `uart1` device `compatible` property is specified like this

```
compatible = "brcm,bcm2835-aux-uart";
```

If you search for `bcm2835-aux-uart` in the Linux source code, you can find a matching driver, it is defined in [8250_bcm2835aux.c](https://github.com/torvalds/linux/blob/v4.14/drivers/tty/serial/8250/8250_bcm2835aux.c)

### Conclusion

You can think about this chapter as a preparation for reading `arm64` boot code - without understanding the concepts that we've just explored you would have a hard time learning it. In the next lesson, we will go back to the `stext` function and examine in details how it works.

