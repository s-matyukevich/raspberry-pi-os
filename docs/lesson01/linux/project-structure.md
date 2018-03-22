
## Lesson 1: Linux project structure

This is the first time in this guide we are going to talk about linux kernel. The idea is to complte some small practicall stem in writing your own kernel and then take a look how the same things are implemented in linux. So far we have don't very little: we just implemented our first bare metal hello world problem, Still we will be able to find some similarities between rpi-os and linux. And now we are going to explore some of them. 

### Project structure

Whenever you start exploring any large software project it worth take a quick look on the project structure. this is very important, because it allows you to understand from which modules the project consist of and what is the high-level architecture. Let's try to explore project structure of linux kernel.

First of all you need to clone the linux repository

```
git clone https://github.com/torvalds/linux.git 
cd linux
git checkout v4.14
```

We are using `v4.14` version, because this was the latest linux version at the time of writing. All references to the linux source code are going to be made for this specific version.

Next, let's take a look on the folders, that we can find inside linux repository. We are not going to look into all of them, but only on those that I consider the most important to start with:

[arch](https://github.com/torvalds/linux/tree/v4.14/arch) - this folder contains subfoldera, each for a specific processor architecture. Mostly we are going to work with [arm64](https://github.com/torvalds/linux/tree/v4.14/arch/arm64) architecture - this is the one that is designed to work with ARM.v8 processors.
[init](https://github.com/torvalds/linux/tree/v4.14/init) - kernel is always booted by  some architecture specific code. But then execution is passed to the [start_kernel](https://github.com/torvalds/linux/blob/v4.14/init/main.c#L509) function that is responsible bo common kernel initialization and actually is an architecture independent kernel starting point. `start_kernel` function togeter with some other initialization functions are defined in the `init` folder.
[kernel](https://github.com/torvalds/linux/tree/v4.14/kernel) - this is the core of the linux kernel. Here you can find implementations of almost all mojor kernel subsystems.
[mm](https://github.com/torvalds/linux/tree/v4.14/mm) - all data structures and methods related to memory management. 
[drivers](https://github.com/torvalds/linux/tree/v4.14/drivers) - this is the largest folder in the linux kernel. It contains implementation of all device drivers.
[fs](https://github.com/torvalds/linux/tree/v4.14/fs) - look here to find implementation of different filesystems.
