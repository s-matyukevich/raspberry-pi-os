## 1.5: Exercises.

Exercises are optional, though I strongly recommend you to experiment with the source code a little bit. If you were able to complete any of the exercises - please share your source code with others. For details see the [contribution guide](../Contributions.md)

1. Introduce a constant `baud_rate`, calculate necessary Mini UART register values using this constant. Make sure that the program can work using baud rates other than 115200.
1. Change the OS code to use UART device instead of Mini UART. Use `BCM2835 ARM Peripherals` manual to figure out how to access UART registers and how to configure GPIO pins.
1. Try to use all 4 processor cores. The OS should print `Hello, from processor <processor index>` for all of the cores. Don't forget to set up a separate stack for each core and make sure that Mini UART is initialized only once. You can use a combination of global variables and `delay` function for synchronization.
1. Adapt lesson 01 to run on qemu. Check [this](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue for reference.

