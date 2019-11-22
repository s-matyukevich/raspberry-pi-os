## 2.3: Exercises

1. Instead of jumping directly from EL3 to EL1, try to get to EL2 first and only then switch to EL1. 
1. One issue that I ran into when working on this lesson was that if FP/SIMD registers are used then everything works well at EL3, but as soon as you get to EL1 print function stops working. This was the reason why I've added [-mgeneral-regs-only](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/Makefile#L3) parameter to the compiler options. Now I want you to remove this parameter and reproduce this behavior. Next, you can use `objdump` tool to see how exactly gcc make use of FP/SIMD registers in the absence of `-mgeneral-regs-only` flag. Finally, I want you to use 'cpacr_el1' to allow using FP/SIMD registers.
1. Adapt lesson 02 to run on qemu. Check [this](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue for reference.

##### Previous Page

2.2 [Processor initialization: Linux](../../docs/lesson02/linux.md)

##### Next Page

3.1 [Interrupt handling: RPi OS](../../docs/lesson03/rpi-os.md)
