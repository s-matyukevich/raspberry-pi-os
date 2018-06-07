## 3.5: Exercises

1. Use local timer instead of the system timer to generate processor interrupts.
1. Handle MiniUART interrupts. Replace the final loop in the `kernel_main` function with a loop that does nothing. Setup MiniUART device to generate an interrupt as soon as the user types a new character. Implement an interrupt handler that will be responsible for printing each newly arrived character on the screen.
1. Adapt lesson 03 to run on qemu. Check [this](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue for reference.
