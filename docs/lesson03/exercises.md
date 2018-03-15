## Lesson 3: Exercises

1. Use processor local timer instead of system global timer to generate processor interrupts.
1. Handle MiniUART interrupts. Replace finall loop in the `kernel_main` function with a loop that actually does nothing. Setup MiniUART device to generate an interrupt as soon as new character arrives. Implement an interrupt handler that will be responsible for printing newely arrived character on the screen.
