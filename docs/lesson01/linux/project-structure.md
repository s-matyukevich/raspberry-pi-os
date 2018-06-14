## 1.2: Linux project structure

This is the first time we are going to talk about Linux. The idea is first to complete some small step in writing our own kernel, and then take a look at how the same things work in Linux. So far we have done very little: we just implemented our first bare metal hello world program, Still, we will be able to find some similarities between the RPi OS and Linux. And now we are going to explore some of them. 

### Project structure

Whenever you start investigating any large software project, it worth taking a quick look at the project structure. This is very important because it allows you to understand what modules compose the project and what is the high-level architecture. Let's try to explore project structure of the Linux kernel.

First of all, you need to clone the Linux repository

```
git clone https://github.com/torvalds/linux.git 
cd linux
git checkout v4.14
```

We are using `v4.14` version because this was the latest version at the time of writing. All references to the Linux source code will be made using this specific version.

Next, let's take a look at the folders that we can find inside the Linux repository. We are not going to look at all of them, but only at those that I consider the most important to start with.

* [arch](https://github.com/torvalds/linux/tree/v4.14/arch) This folder contains subfolders, each for a specific processor architecture. Mostly we are going to work with [arm64](https://github.com/torvalds/linux/tree/v4.14/arch/arm64) - this is the one that is compatible with ARM.v8 processors.
* [init](https://github.com/torvalds/linux/tree/v4.14/init) Kernel is always booted by architecture specific code. But then execution is passed to the [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) function that is responsible for common kernel initialization and is an architecture independent kernel starting point. `start_kernel` function, together with some other initialization functions, is defined in the `init` folder.
* [kernel](https://github.com/torvalds/linux/tree/v4.14/kernel) This is the core of the Linux kernel. Almost all major kernel subsystems are implemented there.
* [mm](https://github.com/torvalds/linux/tree/v4.14/mm) All data structures and methods related to memory management are defined there. 
* [drivers](https://github.com/torvalds/linux/tree/v4.14/drivers) This is the largest folder in the Linux kernel. It contains implementations of all device drivers.
* [fs](https://github.com/torvalds/linux/tree/v4.14/fs) You can look here to find different filesystem implementations.

This explanation is very high-level, but this is enough for now. In the next chapter, we will try to examine Linux build system in some details. 

##### Previous Page

1.1 [Kernel Initialization: Introducing RPi OS, or bare metal "Hello, world!"](../../../docs/lesson01/rpi-os.md)

##### Next Page

1.3 [Kernel Initialization: Kernel build system](../../../docs/lesson01/linux/build-system.md)
