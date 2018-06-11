## 4.5: Exercises

1. Add `printf` to all main kernel functions to output information about the curent memory and processor state. Make sure that the state diagrams, that I've added to the end of the RPi OS part of this lesson, are correct.  (You do not necessarily need to output all state each time, but as soon as some major event happens you can output current stack pointer, or address of the object that has just been allocated, or whatever you consider necessary. Think about some mechanism to prevent information overflow) 
1. Introduce a way to assign priority to the tasks. Make sure that a task with higher priority gets more processor time that the one with lower priority.
1. Allow user processes to use FP/SIMD registers. Those registers should be saved in the task context and swapped during the context switch.
1. Allow the kernel to have an unlimited number of tasks (right now the limit is 64). 
1. Adopt lesson 04 to run on qemu. Check [this](https://github.com/s-matyukevich/raspberry-pi-os/issues/8) issue for reference.

##### Previous Page

4.4 [Process scheduler: Scheduler](../../docs/lesson04/linux/scheduler.md)

##### Next Page

5.1 [User processes and system calls: RPi OS](../../docs/lesson05/rpi-os.md)
