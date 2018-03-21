## Lesson 1: Exercises.

Exercises are optional, though I strongly recomend you to experiment with the source code a little bit. If you was able to successfully complete any of the exercises - please share your source code with others. For details see  [contribution guide](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/Contributions.md)

1. Introduce a constant `baund_rate`, calculate necessary Mini UART register values using this constant. Make sure that the program can work using other baund rate then 115200.
1. Change the OS code to use UART device instead of Mini UART. Use `BCM2835 ARM Peripherals` manual to figure out how to access UART registers and how to configure GPIO pins.
1. Try to use all 4 processors. The OS should print `Hello, from prcessor <processor index>` for all of the processors. Don't forget to setup a separate stack for each processor and make sure that Mini UART is initialized only once. You can use a combination of global variables and `delay` function for syncronization.
