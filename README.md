# Learning operating system development using Linux kernel and Raspberry Pi

This repository contains a step-by-step guide that teaches how to create a simple operating system (OS) kernel from scratch. I call this OS Raspberry Pi OS or just RPi OS. The RPi OS source code is largely based on [Linux kernel](https://github.com/torvalds/linux), but the OS has very limited functionality and supports only [Raspberry PI 3](https://www.raspberrypi.org/products/raspberry-pi-3-model-b/). 

Each lesson is designed in such a way that it first explains how some kernel feature is implemented in the RPi OS, and then it tries to demonstrate how the same functionality works in the Linux kernel. Each lesson has a corresponding folder in the [src](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src) directory, which contains a snapshot of the OS source code at the time when the lesson had just been completed. This allows to introduce new concepts gracefully and helps readers to follow the evolution of the RPi OS. Understanding this guide doesn't require any specific OS development skills.

For more information about project goals and history, please read the [Introduction](docs/Introduction.md). The project is still under active development, if you are willing to participate - please read the [Contribution guide](docs/Contributions.md).

<a href="https://twitter.com/RPi_OS">
  <img src="https://raw.githubusercontent.com/s-matyukevich/raspberry-pi-os/master/images/rpi.png" alt="Follow @RPi_OS" height="20">
</a>

## Table of Content

* **[Introduction](docs/Introduction.md)**
* **[Contribution guide](docs/Contributions.md)**
* **[Prerequisites](docs/Prerequisites.md)**
* **Lesson 1: Kernel Initialization** 
  * 1.1 [Introducing RPi OS, or bare metal "Hello, world!"](docs/lesson01/rpi-os.md)
  * Linux
    * 1.2 [Project structure](docs/lesson01/linux/project-structure.md)
    * 1.3 [Kernel build system](docs/lesson01/linux/build-system.md) 
    * 1.4 [Startup sequence](docs/lesson01/linux/kernel-startup.md)
  * 1.5 [Exercises](docs/lesson01/exercises.md)
* **Lesson 2: Processor initialization**
  * 2.1 [RPi OS](docs/lesson02/rpi-os.md)
  * 2.2 [Linux](docs/lesson02/linux.md)
  * 2.3 [Exercises](docs/lesson02/exercises.md)
* **Lesson 3: Interrupt handling**
  * 3.1 [RPi OS](docs/lesson03/rpi-os.md)
  * Linux
    * 3.2 [Low level exception handling](docs/lesson03/linux/low_level-exception_handling.md) 
    * 3.3 [Interrupt controllers](docs/lesson03/linux/interrupt_controllers.md)
    * 3.4 [Timers](docs/lesson03/linux/timer.md)
  * 3.5 [Exercises](docs/lesson03/exercises.md)
* **Lesson 4: Process scheduler**
  * 4.1 [RPi OS](docs/lesson04/rpi-os.md) 
  * Linux
    * 4.2 [Scheduler basic structures](docs/lesson04/linux/basic_structures.md)
    * 4.3 [Forking a task](docs/lesson04/linux/fork.md)
    * 4.4 [Scheduler](docs/lesson04/linux/scheduler.md)
  * 4.5 [Exercises](docs/lesson04/exercises.md)
* **Lesson 5: User processes and system calls** 
  * 5.1 [RPi OS](docs/lesson05/rpi-os.md) 
  * 5.2 [Linux](docs/lesson05/linux.md)
  * 5.3 [Exercises](docs/lesson05/exercises.md)
* **Lesson 6: Virtual memory management**
  * 6.1 [RPi OS](docs/lesson06/rpi-os.md) 
  * 6.2 Linux (In progress)
  * 6.3 [Exercises](docs/lesson06/exercises.md)
* **Lesson 7: Signals and interrupt waiting** (To be done)
* **Lesson 8: File systems** (To be done)
* **Lesson 9: Executable files (ELF)** (To be done)
* **Lesson 10: Drivers** (To be done)
* **Lesson 11: Networking** (To be done)

