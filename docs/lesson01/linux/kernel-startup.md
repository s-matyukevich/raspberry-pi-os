## Lesson 1: Linux startup sequence.

### Searching for entrypoint.

After taking a look on project structure and examining the project can be built, next logical step is to find the program entrypoint. This step might be trivial for a lot of programs, but not for linux kernel. 

The first file we are going to take a look at is [arm64 linker script](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S). We have already seen how linker script [is used in the main Makefile](https://github.com/torvalds/linux/blob/v4.14/Makefile#L970). From this line we can eaqsily infer, where the linker script for a particular architecture is located.

It should be mentioned that the file, that we are going to examine is not an actual linker script - it is a template, from which actual linker script is buit by substituting some macros with their actual values. But presisely because this file consist mostly of macros (similar to [this one](https://github.com/torvalds/linux/blob/v4.14/include/asm-generic/vmlinux.lds.h#L514)) it content becomes much easier to read and to port between different architectures.

The fist section that we can find in the linker script is called [.head.text](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/vmlinux.lds.S#L96) This is very important for us, because our entry point should be defined in this section. If you think a liite about it, it makes a perfect sense: when kernel is loaded the content of the binary image is copied into some memory area and execution is started from the beggining of that area. This means that just by searching wich files uses `.head.text` section we should be able to find the entrypoint. And indeed, `arm64` architecture has a single file the uses [__HEAD](https://github.com/torvalds/linux/blob/v4.14/include/linux/init.h#L90) makro which is expanded to `.section    ".head.text","ax"` - this file is [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S)

The first executable line, that we can find in `head.S` file is [this one](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L85). Here we use arm assembler `b` of `branch` instruction to jump to the [stext](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L116) function. And here it is - this is the fist function that gets executed when you boot your kernel.

At this point next logincal step would be to explore what is going on inside `stext` function - but we are not ready yet. First we are going to implement similar functionality in our own kerne, and that is something we are going to do in the next few lessons. What we are going to do right now is to examine a few important concepts, related to kernel boot.

### Linux bootloader and boot protocol.

When  linux kernel is booted it assumes that machine hardware is initialized to some "known state". The set of rules, that defines this state is called "boot protocol" and for `arm64` architecture it is documented [here](https://github.com/torvalds/linux/blob/v4.14/Documentation/arm64/booting.txt). Among other things it defines, for example, that execution must start only on primary CPU, Memory Mapping Unit must be turned of and all interupts must be masked. 

Ok, but who is responsible bor bringing a machin into that known state? Usually thre is a special program that runs befor the kernel and performs all initializations. This prgram is called `bootloader`. Bootloader code may be very machine specific. This is the case, for example, with Raspberry PI. On Raspberry PI bootloader is bootloader is built into the board. We can only use [config.txt](https://www.raspberrypi.org/documentation/configuration/config-txt/) file to customize its behaviour. 

### UEFI boot

However there is one boot loader that can be built into that kernel image itself. This bootloader can be used only on the platforms that support [Unified Extensible Firmware Interface (UEFI)](https://en.wikipedia.org/wiki/Unified_Extensible_Firmware_Interface). Devices that suport UEFI provides a set of standartized services to the running software and those services can be used  to figure out all necesary information about device capabilities and characteristics. UEFI also requires that computer firmware should be cappable of running executable files in [Portable Executabe (PE)](https://en.wikipedia.org/wiki/Portable_Executable) format. Linux kernel UEFI bootloader make use of this feature: it injects `PE` header at the beggining of linux kernel image, so that computer fimware think that the image is a normal `PE` file. This is done in [efi-header.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S) file. This file defines [__EFI_PE_HEADER](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L13) macro that is used inside [head.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/head.S#L98).

One important thing that is defined inside `__EFI_PE_HEADER` is a [defenition of UEFI entrypoint](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-header.S#L33) and not suprisingly, the entry point is defined in [efi-entry.S](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/kernel/efi-entry.S#L32). From this point you can follow the source code and examine what exactly UEFI boot loader is doing, if you are interested in such details (it is more or less straightforward). But we are going to stop here, because the purpose of this section was not to examine UEFI bootloader, but to give you a general idea what UEFI is and how linux kernel uses it.

### Device Tree

When I started to examinie startup code of linux kernel I found a lot of mentionins of so calle `Device Tree`. It appears to be a very important concept and I consider it necessary to brefely discuss it.

When we were working on `Raspberry PI OS` kernel we used [BCM2835 ARM Peripherals manual](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf) to figure out what is the exact offset at which a particular memory mapped register is located. This information is different for each board, and we are lucky that we are going to support only one of them. But what if we need to suport thousands of boards? If would be a total mess if we try to hardcode such king of information in the kernel itself. And even if we manage to do so, how would we figure out what board we are using right now? `BCM2835`, for exmaple, don't provide any means of communicating such kind of information to the running kernel.

Device tree provides us with the solution to the probems, mentioned above. It is actually a specification of a format that should be used to describe computer hardware. The specification itself can be found [here](https://www.devicetree.org/). When kernel is executed boot loader selects proper device tree file and passes it as an agument to the kernel. If you take a look at the files in the boot partition on a Raspberry PI sd card after installation of a [Raspbian](https://www.raspberrypi.org/downloads/raspbian/) OS is finished, you can find a lot of `.dtb` files here. `.dtb` files are just compiled device tree files. You can select some of them in our `config.txt` in order to enable or disable some Raspberry PI  hardware. This process is described in more details in the [Raspberry PI oficial documentation](https://www.raspberrypi.org/documentation/configuration/device-tree.md)

Ok, now it is time  to take a look how a device tree looks like. As an execrice let's try to find a device tree for [Raspberry PI 3 Model b](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/) - a device that we are curently using. From the [documentation](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2837/README.md) we can learn that `Raspberry PI 3 Model b` uses a chip that is called `BCM2837`. If you search for this name you can find [/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm64/boot/dts/broadcom/bcm2837-rpi-3-b.dts)  file. As you might see it just includes the same file from `arm` architecture. This make a perfect sense, because `ARM.v8` processor suports both 32 and 64 bit mode and both `arm` and `arm64` architectures can be run on the same device. 

It is easy to find [bcm2837-rpi-3-b.dts](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts) for the [arm](https://github.com/torvalds/linux/tree/v4.14/arch/arm) architecture. We already saw that device tree files can include on another. This is the case with the  `bcm2837-rpi-3-b.dts` - it only conly contains those definitions, that are specific for `BCM2837`. For example, it specifies that [the device now have 1GB of memory](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L18). 

As I mentioned previously, `BCM2837` and `BCM2835` have identical periferial hardware and if you follow the chain of includes you can find [bcm283x.dtsi](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi) that actualy specifies most of this hardware. 

A device tree definition consist of the blocks nested one in another. At the top level we usually can find such block as [cpus](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837.dtsi#L30) or [memory](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm2837-rpi-3-b.dts#L17) The meanin of thouse block  should be quite self explanatory. Another interesting top level element, that we can find in the `bcm283x.dtsi` is [sos](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L52) that means [System on a chip](https://en.wikipedia.org/wiki/System_on_a_chip) It tels us that all periferal devices are directly mapped to some memory area via memory mapped registers. `soc` element serves as a parent element for all peripheral devices. One of its children is [gpio](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L147) element. This element defines [reg = <0x7e200000 0xb4>](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L147) property that tells us that gpio memory mapped reristers are located in the area from `0x7e200000` to `0x7e2000b4`. One of the childern of `gpio` element is defined like this

```
uart1_gpio14: uart1_gpio14 {
        brcm,pins = <14 15>;
        brcm,function = <BCM2835_FSEL_ALT5>;
};
``` 
This definition tells us that if alternative function 5 is selected for pins 14 and 15 those pins will be connection to `uart1` device. You can easily gues that `uart1` device is Mini UART that we have used already, this device definition can be found [here](https://github.com/torvalds/linux/blob/v4.14/arch/arm/boot/dts/bcm283x.dtsi#L474)

One important thing, that you need to know about device trees is that its format is extendable. For each device its own properties and nested blocks can be defined. Those properties are transparently passed to the device driver, and it is driver responsibility to understand them. But how can the kernel figure out the corespondance betweend blocks in the  device tree and drivers? It uses `compatible` property to do this. For example, for `uart1` device `compatible` property is defined like this

```
compatible = "brcm,bcm2835-aux-uart";
```

If you search for `bcm2835-aux-uart` in linux source code indeed you can find a maching driver, it is defined in [8250_bcm2835aux.c](https://github.com/torvalds/linux/blob/v4.14/drivers/tty/serial/8250/8250_bcm2835aux.c)

