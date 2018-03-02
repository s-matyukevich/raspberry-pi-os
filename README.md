# Learning operating system development using Linux kernel and Raspberry Pi

This repository contains a step-by-step guide that teaches how to create a simple operating system (OS) from scratch. I call this OS Raspberry Pi OS or just RPi OS. The source code for the RPi OS is largely based on [Linux kernel](https://github.com/torvalds/linux), and it supports only [Raspberry PI 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/) device. 

Each lesson is designed in such a way that it first explains how some kernel feature is implemented in the RPi OS, and then it tries to demonstrate how the same functionality works in the Linux kernel. Each lesson has a corresponding folder in the [src](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src) directory, which contains a snapshot of the OS source code at the time when the lesson had just been completed. This allows to introduce new concepts gracefully and helps readers to follow the evolution of the RPi OS. Understanding this guide doesn't require any specific OS development skills.

For more information about the project goals and history, please read the [Introduction](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/Introduction.md). The project is still under active development, if you are willing to participate - please read the [Contribution guide](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/Contributions.md).

## Table of Content

* **[Introduction](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/Introduction.md)**
* **[Contribution guide](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/Contributions.md)**
* **[Prerequsities](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/Prerequisites.md)**
* **Lesson 1: Kernel Initialization** 
  * 1.1 [Introducing RPi OS, or bare metal "Hello, world!"](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/rpi-os.md)
  * Linux
    * 1.2 [Project structure](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/project-structure.md)
    * 1.3 [Kernel build system](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/build-system.md) 
    * 1.4 [Startup sequence](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/kernel-startup.md)
  * 1.5 [Exercises](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson01/exercises.md)
* **Lesson 2: Processor initialization**
  * 2.1 [RPi OS](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson02/rpi-os.md)
  * 2.2 [Linux](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson02/linux.md)
  * 2.3 [Exercises]()
* **Lesson 3: Interrupt handling**
  * 3.1 [RPi OS](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson03/rpi-os.md)
  * Linux
    * 3.2 [Low level exception handling](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson03/low_level-exception_handling.md) 
    * 3.3 [Interrupt controllers](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson03/interrupt_controllers.md)
    * 3.4 [Timers](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson03/timer.md)
  * 3.5 [Exercises]()
* **Lesson 4: Process scheduler**
  * 4.1 [RPi OS](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/lesson04/rpi-os.md) 
  * 4.2 [Linux]()
  * 4.3 [Exercises]()
* **Lesson 5: User processes and system calls** 
  * 5.1 [RPi OS]() 
  * 5.2 [Linux]()
  * 5.3 [Exercises]()
* **Lesson 6: Memory management**
  * 6.1 [RPi OS]() 
  * 6.2 [Linux]()
  * 6.3 [Exercises]()
* **Lesson 7: File systems** (To be done)
* **Lesson 8: Executable files (ELF)** (To be done)
* **Lesson 9: Networking** (To be done)

