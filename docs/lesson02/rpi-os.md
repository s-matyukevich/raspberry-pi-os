## Lesson 2: Processor initialization

In this lesson we are going to familiarizze yourself with some important concepts, relevant to ARM processors. Understanding of those concept is important in case you want to fully utilize ARM proessor capabilities. So now let's take a breef lool on the most important of them.

### Exception levels

Each ARM processor that suports ARM.v8 architecture has 4 exception levels. You can think about exception level as a processor execution mode in withc some operations are ether enabled  or disabled. The leastpriviliged exception level is level 0. When processor operates at this level it mostly uses only general purpose registers (X0 - X30) and stack pointer register (SP). Exception level 0 also allows us to use STR and LDR instructions to load and store data to and from memory. 

An operating system should deal with exception levels, because it needs to implement process isolation. Each user process should not be able to access any data that belongs to different user process. In order to achive such behaviour, an operating system always runs each user process at exception level 0. Operating at this exception level a process can only use it's own virtual memory and can't access any instruction that changes virtual memory settings. So in order to ensure process isolation, an OS just need to prepare separate virtual memory mapping for each process and put processor into exception level 0 before executing any user process.

Operating system itself usually operates at exception level 1. While running at this exception level processor gets access to the registers that allows to configure virtual memory settings as well as to some system registers. Raspberry Pi OS also will be using exception level 1.

We are not going to use exceptions levels 2 and 3 a lot, but I just wanted to brefely describe them so you can get an idea why they are needed. 

Exception level 2 is needed in a scenario when we are using a hypervisor. In this case host operating system runs on exception level 2 and guest operating systems can only use EL 1. This allows host OS to isolate guest OSes from each other in a similar way how OS isolates user proceses.

Exception level 3 is used for transitions from ARM "Secure World" to "Unsecure world". Those concepts exists to provide full hardware isolation between software running in different "worlds". Application from an "Unsecure World" can in no way access or modify information (both instruction and data) that belongs to "Secure world" and this restriction is enforced on a hardware level. 

### Debugging the kernel

At this point next logical step would be to figure uat what Exception level we are curently using. But when I tried to do this I realized that my kernle can only pring some constant string on a scren, and what I need is some analog of [printf](https://en.wikipedia.org/wiki/Printf_format_string) function. With `printf` I can easily display values of different registers and variables. Such posibility is esential for kernel development, because you don't have any other debugger support and `printf` becomes the only mean by which you can figure out what is going on inside your program.

For my own kernel I decided not to reinvert the wheel and use one of  [existing implementations](http://www.sparetimelabs.com/tinyprintf/tinyprintf.php) Indeed, this function consists mostly from string manipulations and is not very interesting from kernel development point of view. The implementation that I used is very small and don't have external dependencies, that alows it to be easily integrated inside my kernel. The only thing that I have to do is to define `putc`  function that can send a single character on the screen. This funciton is defined [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/mini_uart.c#L59) and it just uses already defined `uart_send` function. Also we need to initialize `printf` library and specify the location of the `putc` function. This is done in a single [line of code](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L8)

### Finding out current Exception level

Now when we are equiped with `printf` function we can complete our original task: figure out at which exception level we are curently booted. A small function that can answer this question is defined [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/utils.S#L1) and looks like this

```
.globl get_el
get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret
```

Here we use `mrs` instruction to read value from `CurrentEL` system register into `x0` register. Then we shift this value 2 bits to the right (we need to do this because first 2 bits in the `CurrentEL` register are reserved and always have value 0) And finally in the register `x0` we have an integer nuber indicating current exception level. Now the only thing that is left is to display this value, like [this](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/src/kernel.c#L10)

```
	int el = get_el();
	printf("Exception level: %d \r\n", el);
```

If you make everything right you should see `Exception level: 3` on the screen.

### Changing current exception level

In ARM architecture there is no way how a progam can increase it's own exception level without participation of the software that already runs on higer level. This makes a perfect sense: otherwise any program would be able to escape its owhn exception level and access other programs data. The only thing a program can do is to generate an exception. An exception is generated whenever a program executes some illegal instruction (for example, tries to access memory location at address, that doesn't exist, or tries to divide by 0), or a program can execute `svc` instruction to generate exception on purpose if it needs to. Hardware generated interrupts are also handled  as a special types of exceptions. Whenever an exception is generated the following sequence of steps taks place (In the description I will assume that exception is handled at exception level `n`).

1. Address of the instruction next to the curent one is saved in the `ELR_ELn`  register. (It is called `Exception link register`)
1. Current processor state (`pstate` register) is stored in `SPSR_ELn` register (`Saved Program Status Register`)
1. Exception handler is executed and does whatever job it needs to do.
1. Exception handler calls `eret` instruction. This instruction restores processor state from `SPSR_ELn` and resums execution starting from the address, stored in the `ELR_ELn`  register.

On practise the process is a little more complicated, because exception handler also need to store state of all general purpose registers and restore them back ater it finishes execution, but we will discuss this process in details in the next lesson. For now we need just to understand the process in general and remember meaning of the `ELR_ELm` and `SPSR_ELn` registers.

Important thing to know is that exception handler is not obliged to return to that same location from which the exception had taken place. Both `ELR_ELm` and `SPSR_ELn` are writable and exception handler can modify them if it want. We are goingto use this feature to our advantage when we try to switch from EL3 to EL1 in our code.

### Switching to EL1

Strictly speaking, our operating system is not obliged to switch to EL1, but EL1 is a natural chois for us because at this level we are going to have just the right set of privileges to implement common OS tasks. It also will be an interesting exercise to see how switching exceptions levels works in action. Let's take a loot at the source code that does this.

```
master:
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0		

	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0

	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0

	ldr	x0, =SPSR_VALUE
	msr	spsr_el3, x0

	adr	x0, el1_entry		
	msr	elr_el3, x0

	eret				
```

As you can see the conde consists mostly from configuring values of some system registers. Now we are going to examine those registers one by one. In order to do this we first need to download [AArch64-Reference-Manual](https://developer.arm.com/docs/ddi0487/latest/arm-architecture-reference-manual-armv8-for-armv8-a-architecture-profile) This document contains detailed specification of the `ARM.v8` architecture. 

#### SCTLR_EL1, System Control Register (EL1), Page 2025 of AArch64-Reference-Manual.

```
	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0		
```

Here we set value of the `sctlr_el1` system register. `sctlr_el1` is responsible for configureing different parameters of the processor when it operates at EL1. For example, it controls whenter cache is enabled and what is most important for us whether MMU (Memory Mapping Unit) is turned on. `sctlr_el1` is accessible from all exception levels higer or equal then EL1 (you can infer this from `_el1` postfix) 

`SCTLR_VALUE_MMU_DISABLED` constant is defined [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L16) Individual bits of this value are defined like this:

* `#define SCTLR_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)` - Some bits in the description of `sctlr_el1` register are marked as `RES1`. Those bits are reserved for future ussage and should be initialized with `1`.
* `#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)` - Exception [Endianness](https://en.wikipedia.org/wiki/Endianness) This field controls endianess of explicit data access at EL1. We are going to configure the processor to work only with `little-endian` format.
* `#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)` - Similar to previous field but this one controls endianess of explicit data access at EL0, instead of EL1. 
* `#define SCTLR_I_CACHE_DISABLED          (0 << 12)` - Disable instruction cache. We are going to disable all caches for simplicity. You can find more information about data and instruction caches [here](https://stackoverflow.com/questions/22394750/what-is-meant-by-data-cache-and-instruction-cache).
* `#define SCTLR_D_CACHE_DISABLED          (0 << 2)` - Disable data chache.
* `#define SCTLR_MMU_DISABLED              (0 << 0)` - Disable MMU. MMU must be disabled until the lesson 6, where we are going to prepare page tables and start working with virtual memory.
* `#define SCTLR_MMU_ENABLED               (1 << 0)` - We will use this field later when we need to enable MMU.

#### HCR_EL2, Hypervisor Configuration Register (EL2), Page 1923 of AArch64-Reference-Manual. 

```
	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0
```

We are not going to implement our own [hypervisor](https://en.wikipedia.org/wiki/Hypervisor). Stil we need to use this register, because among other setings it controls execution state at EL1. Execution state must be `AArch64` and not `AArch32`. This is configured [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L22) 

#### SCR_EL3, Secure Configuration Register (EL3), Page 2022 of AArch64-Reference-Manual.

```
	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0
```

This register is responsible for configuring security settings. For example, it controls wheter all lower levels are executed in "secure" or "non secure" state. It also controls execution state at EL2. [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L26) we set that EL2  will execute at `AArch64` state, and all lower exception levels will be "non secure". 

#### SPSR_EL3, Saved Program Status Register (EL3), Page 288 of AArch64-Reference-Manual.

```
	ldr	x0, =SPSR_VALUE
	msr	spsr_el3, x0
```

This one should be already familiar to you, we mentioned it when discussed the process of changing exception levels. `spsr_el3` contains progessor  state, that is going to be restored after we execute `eret` instruction.
It worth saying a few words explaining what processor state is. Processor state includes the following information:

* _Condition Flags_ - Those flags contains information about previously executed opeartion, wheter the result was neagtive (N flag), zero (A flag), has unsigned overflow (C flag) or has signed overflow (V flag). Values of those flags can be used in conditional branch instructions. For example, `b.eq` instruction will jump to the provided label only if the result of the last comparison operation is equal to 0. The processor verifyes this by checking whether Z flag is set to 1.

* _Interrupt disable bits_ - Those bits allows to enable/disable different types of interrupts.

* All other information, required to fully restore processor executin state after exception is hanled.

Usually `spsr_el3` is saved automatically whenever exception is taken to EL3. However this register is writable, so we take advantage of it and manually prepare processor state. `SPSR_VALUE` is prepared [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson02/include/arm/sysregs.h#L35) and we initialize the following fields:

* `#define SPSR_MASK_ALL        (7 << 6)` - After we change EL to EL1 all types of interrupts will be masked.
* `#define SPSR_EL1h		(5 << 0)` - At EL1 we can either use our own dedicated stack pointer or use the same stack pointer that is used at EL0. `EL1h` mode mode means that we are using EL1 dedicated stack pointer. 

#### ELR_EL3, Exception Link Register (EL3), Page 260 of AArch64-Reference-Manual.

```
	adr	x0, el1_entry		
	msr	elr_el3, x0

	eret				
```

`elr_el3` hlds an address, to which we are going to return after `eret` instruction will be executed. Here we set this address to the location of `el1_entry` label.

That is pretty much it: when we enter `el1_entry` function the execution should be already at EL1 mode.
