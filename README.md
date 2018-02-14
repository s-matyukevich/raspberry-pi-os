## Learning operating system development using Linux kernel and Raspberry Pi.

This repository contains a tep-by-step guide that teaches how to create your own simple operating system from scratch. I call it Raspberry Pi OS of just RPi OS. The source code for the RPi OS is largely based on [Linux kernl](https://github.com/torvalds/linux) and it is desined to be run only on [Raspberry PI 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/) device. 

Each lesson is designed  such that it first teaches how some kernel feature is implemented in RPi OS, and then it tries to explain how the same feature works in linux kernel. Source code for each lesson is stored in a separeate directory so that readers can easily follow the evolution of the code. In order to understand this guid you don't need any specific OS development skills - all lessons explain all new conceps in as much details as posible.

For more information about the project goals and history please read introduction. The project is still under an active devlopment, if you want to participate - please read the contribution guide.

## Table of Content

* Introduction
* Contribution guide
* Prerequsities
* Lesson 1:Kernel Initialization 
  1.1 Introducing RPi OS, or bare metal "Hello, world!".
  1.2 Linux project structure. 
  1.3 Kernel build system. 
  1.4 Linux startup sequence. 
  1.5 Exercises.
* Lesson 2: Processor initialization.
  2.1 Raspberry PI OS.
  2.2 Linux kernel. 
  2.3 Exercises.
* Lesson 3: Interrupt handling.
  3.1 Raspberry PI OS.
  3.2 Low level exception handling in linux.
  3.3 How linux drivers handles interrupts.
  3.4 Exercises.
* Lesson 4: System calls.
  4.1 Raspberry PI OS.
  4.2 Linux kernel. 
  4.3 Exercises.
* Lesson 5: Process scheduler.
  5.1 Raspberry PI OS.
  5.2 Linux kernel. 
  5.3 Exercises.
* Lesson 6: Memory management.
  6.1 Raspberry PI OS.
  6.2 Linux kernel. 
  6.3 Exercises.
* Lesson 7: File systems. (To be done).
* Lesson 8: Executable files (ELF) (To be done).
* Lesson 9: Networking. (To be done).
