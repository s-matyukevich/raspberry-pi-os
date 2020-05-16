## 2.1: Processor initialization 

In this lesson, we are going to work more closely with the ARM processor. It has some essential features that can be utilized by the OS. The first such feature is called "Exception levels".

### Exception levels

Each ARM processor that supports ARM.v8 architecture has 4 exception levels. You can think about an exception level (or `EL` for short) as a processor execution mode in which only a subset of all operations and registers is available. The least privileged exception level is level 0. When processor operates at this level, it mostly uses only general purpose registers (X0 - X30) and stack pointer register (SP). EL0 also allows using `STR` and `LDR` commands to load and store data to and from memory and a few other instructions commonly used by a user program.

An operating system should deal with exception levels because it needs to implement process isolation. A user process should not be able to access other process's data. To achieve such behavior, an operating system always runs each user process at EL0. Operating at this exception level a process can only use it's own virtual memory and can't access any instructions that change virtual memory settings. So, to ensure process isolation, an OS need to prepare separate virtual memory mapping for each process and put the processor into EL0 before transferring execution to a user process.

An operating system itself usually works at EL1. While running at this exception level processor gets access to the registers that allows configuring virtual memory settings as well as to some system registers. Raspberry Pi OS also will be using EL1.

We are not going to use exceptions levels 2 and 3 a lot, but I just want to briefly describe them so you can get an idea why they are needed. 

EL2 is used in a scenario when we are using a hypervisor. In this case host operating system runs at EL2 and guest operating systems can only use EL 1. This allows host OS to isolate guest OSes in a similar way how OS isolates user processes.

EL3 is used for transitions from ARM "Secure World" to "Insecure world". This abstraction exist to provide full hardware isolation between the software running in two different "worlds". Application from an "Insecure world" can in no way access or modify information (both instruction and data) that belongs to "Secure world", and this restriction is enforced at the hardware level. 

### Debugging the kernel

Next thing that I want to do is to figure out which Exception level we are currently using. But when I tried to do this, I realized that the kernel could only print some constant string on a screen, but what I need is some analog of [printf](https://en.wikipedia.org/wiki/Printf_format_string) function. With `printf` I can easily display values of different registers and variables. Such functionality is essential for the kernel development because you don't have any other debugger support and `printf` becomes the only mean by which you can figure out what is going on inside your program.

For the RPi OS I decided not to reinvent the wheel and use one of  [existing printf implementations](http://www.sparetimelabs.com/tinyprintf/tinyprintf.php) This function consists mostly from string manipulations and is not very interesting from a kernel developer point of view. The implementation that I used is very small and don't have external dependencies, that allows it to be easily integrated into the kernel. The only thing that I have to do is to define `putc`  function that can send a single character to the screen. This function is defined [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/mini_uart.c#L59) and it just uses already existing `uart_send` function. Also, we need to initialize the `printf` library and specify the location of the `putc` function. This is done in a single [line of code](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L8).

### Finding current Exception level

Now, when we are equipped with the `printf` function, we can complete our original task: figure out at which exception level the OS is booted. A small function that can answer this question is defined [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/utils.S#L1) and looks like this.

```
.globl get_el
get_el:
    mrs x0, CurrentEL
    lsr x0, x0, #2
    ret
```

Here we use `mrs` instruction to read the value from `CurrentEL` system register into `x0` register. Then we shift this value 2 bits to the right (we need to do this because first 2 bits in the `CurrentEL` register are reserved and always have value 0) And finally in the register `x0` we have an integer number indicating current exception level. Now the only thing that is left is to display this value, like [this](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L10).

```
    int el = get_el();
    printf("Exception level: %d \r\n", el);
```

If you reproduce this experiment, you should see `Exception level: 2` on the screen.

### Changing current exception level

In ARM architecture there is no way how a program can increase its own exception level without the participation of the software that already runs on a higher level. This makes a perfect sense: otherwise, any program would be able to escape its assigned EL and access other programs data. Current EL can be changed only if an exception is generated. This can happen if a program executes some illegal instruction (for example, tries to access memory location at a nonexisting address, or tries to divide by 0) Also an application can run `svc` instruction to generate an exception on purpose. Hardware generated interrupts are also handled as a special type of exceptions. Whenever an exception is generated the following sequence of steps takes place (In the description I am assuming that the exception is handled at EL `n`, were `n` could be 1, 2 or 3).

1. Address of the current instruction is saved in the `ELR_ELn`  register. (It is called `Exception link register`)
1. Current processor state is stored in `SPSR_ELn` register (`Saved Program Status Register`)
1. An exception handler is executed and does whatever job it needs to do.
1. Exception handler calls `eret` instruction. This instruction restores processor state from `SPSR_ELn` and resumes execution starting from the address, stored in the `ELR_ELn`  register.

In practice the process is a little more complicated because exception handler also needs to store the state of all general purpose registers and restore it back afterwards, but we will discuss this process in details in the next lesson. For now, we need just to understand the process in general and remember the meaning of the `ELR_ELm` and `SPSR_ELn` registers.

An important thing to know is that exception handler is not obliged to return to the same location from which the exception originates. Both `ELR_ELm` and `SPSR_ELn` are writable and exception handler can modify them if it wants to. We are going to use this technique to our advantage when we try to switch from EL3 to EL1 in our code.

### Switching to EL1

Strictly speaking, our operating system is not obliged to switch to EL1, but EL1 is a natural choice for us because this level has just the right set of privileges to implement all common OS tasks. It also will be an interesting exercise to see how switching exceptions levels works in action. Let's take a look at the [source code that does this](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/boot.S#L17).

```
master:
    ldr    x0, =SCTLR_VALUE_MMU_DISABLED
    msr    sctlr_el1, x0        

    ldr    x0, =HCR_VALUE
    msr    hcr_el2, x0

    ldr    x0, =SCR_VALUE
    msr    scr_el3, x0

    ldr    x0, =SPSR_VALUE
    msr    spsr_el3, x0

    adr    x0, el1_entry        
    msr    elr_el3, x0

    eret                
```

As you can see the code consists mostly of configuring a few system registers. Now we are going to examine those registers one by one. In order to do this we first need to download [AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/ca/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile). This document contains the detailed specification of the `ARM.v8` architecture. 

#### SCTLR_EL1, System Control Register (EL1), Page 2654 of AArch64-Reference-Manual.

```
    ldr    x0, =SCTLR_VALUE_MMU_DISABLED
    msr    sctlr_el1, x0        
```

Here we set the value of the `sctlr_el1` system register. `sctlr_el1` is responsible for configuring different parameters of the processor, when it operates at EL1. For example, it controls whether the cache is enabled and, what is most important for us, whether the MMU (Memory Management Unit) is turned on. `sctlr_el1` is accessible from all exception levels higher or equal than EL1 (you can infer this from `_el1` postfix) 

`SCTLR_VALUE_MMU_DISABLED` constant is defined [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L16) Individual bits of this value are defined like this:

* `#define SCTLR_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)` Some bits in the description of `sctlr_el1` register are marked as `RES1`. Those bits are reserved for future usage and should be initialized with `1`.
* `#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)` Exception [Endianness](https://en.wikipedia.org/wiki/Endianness). This field controls endianess of explicit data access at EL1. We are going to configure the processor to work only with `little-endian` format.
* `#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)` Similar to previous field but this one controls endianess of explicit data access at EL0, instead of EL1. 
* `#define SCTLR_I_CACHE_DISABLED          (0 << 12)` Disable instruction cache. We are going to disable all caches for simplicity. You can find more information about data and instruction caches [here](https://stackoverflow.com/questions/22394750/what-is-meant-by-data-cache-and-instruction-cache).
* `#define SCTLR_D_CACHE_DISABLED          (0 << 2)` Disable data cache.
* `#define SCTLR_MMU_DISABLED              (0 << 0)` Disable MMU. MMU must be disabled until the lesson 6, where we are going to prepare page tables and start working with virtual memory.

#### HCR_EL2, Hypervisor Configuration Register (EL2), Page 2487 of AArch64-Reference-Manual. 

```
    ldr    x0, =HCR_VALUE
    msr    hcr_el2, x0
```

We are not going to implement our own [hypervisor](https://en.wikipedia.org/wiki/Hypervisor). Stil we need to use this register because, among other settings, it controls the execution state at EL1. Execution state must be `AArch64` and not `AArch32`. This is configured [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L22).

#### SCR_EL3, Secure Configuration Register (EL3), Page 2648 of AArch64-Reference-Manual.

```
    ldr    x0, =SCR_VALUE
    msr    scr_el3, x0
```

This register is responsible for configuring security settings. For example, it controls whether all lower levels are executed in "secure" or "nonsecure" state. It also controls execution state at EL2. [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L26) we set that EL2  will execute at `AArch64` state, and all lower exception levels will be "non secure". 

#### SPSR_EL3, Saved Program Status Register (EL3), Page 389 of AArch64-Reference-Manual.

```
    ldr    x0, =SPSR_VALUE
    msr    spsr_el3, x0
```

This register should be already familiar to you - we mentioned it when discussed the process of changing exception levels. `spsr_el3` contains processor state, that will be restored after we execute `eret` instruction.
It is worth saying a few words explaining what processor state is. Processor state includes the following information:

* **Condition Flags** Those flags contains information about previously executed operation: whether the result was negative (N flag), zero (A flag), has unsigned overflow (C flag) or has signed overflow (V flag). Values of those flags can be used in conditional branch instructions. For example, `b.eq` instruction will jump to the provided label only if the result of the last comparison operation is equal to 0. The processor checks this by testing whether Z flag is set to 1.

* **Interrupt disable bits** Those bits allows to enable/disable different types of interrupts.

* Some other information, required to fully restore the processor execution state after an exception is handled.

Usually `spsr_el3` is saved automatically when an exception is taken to EL3. However this register is writable, so we take advantage of this fact and manually prepare processor state. `SPSR_VALUE` is prepared [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L35) and we initialize the following fields:

* `#define SPSR_MASK_ALL        (7 << 6)` After we change EL to EL1 all types of interrupts will be masked (or disabled, which is the same).
* `#define SPSR_EL1h        (5 << 0)` At EL1 we can either use our own dedicated stack pointer or use EL0 stack pointer. `EL1h` mode means that we are using EL1 dedicated stack pointer. 

#### ELR_EL3, Exception Link Register (EL3), Page 351 of AArch64-Reference-Manual.

```
    adr    x0, el1_entry        
    msr    elr_el3, x0

    eret                
```

`elr_el3` holds the address, to which we are going to return after `eret` instruction will be executed. Here we set this address to the location of `el1_entry` label.

### Conclusion

That is pretty much it: when we enter `el1_entry` function the execution should be already at EL1 mode. Go ahead and try it out! 

##### Previous Page

1.5 [Kernel Initialization: Exercises](../../docs/lesson01/exercises.md)

##### Next Page

2.2 [Processor initialization: Linux](../../docs/lesson02/linux.md)
