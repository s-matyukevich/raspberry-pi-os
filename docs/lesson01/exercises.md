## 1.5: Exercises.

Exercises are optional, though I strongly recommend you to experiment with the source code a little bit. If you were able to complete any of the exercises - please share your source code with others. For details see the [contribution guide](../Contributions.md).

1. Introduce a constant `baud_rate`, calculate necessary Mini UART register values using this constant. Make sure that the program can work using baud rates other than 115200.
1. Change the OS code to use UART device instead of Mini UART. Use `BCM2837 ARM Peripherals` and [ARM PrimeCell UART (PL011)](http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183g/DDI0183G_uart_pl011_r1p5_trm.pdf) manuals to figure out how to access UART registers and how to configure GPIO pins. The UART device uses the 48MHz clock as a base.
1. Try to use all 4 processor cores. The OS should print `Hello, from processor <processor index>` for all of the cores. Don't forget to set up a separate stack for each core and make sure that Mini UART is initialized only once. You can use a combination of global variables and `delay` function for synchronization.
1. Adapt lesson 01 to run on qemu. Check [this](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue for reference.

##### Previous Page

1.4 [Kernel Initialization: Linux startup sequence](../../docs/lesson01/linux/kernel-startup.md)

##### Next Page

2.1 [Processor initialization: RPi OS](../../docs/lesson02/rpi-os.md)
