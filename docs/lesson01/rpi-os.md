## 1.1: Introducing RPi OS, or bare metal "Hello, world!"

We are going to start our journey to the OS development world by writing a small bare metal "Hello, world" application. I assume that at this time you have gone through the [Prerequisites](docs/Prerequisites.md) and have everything ready for the work. If not - now is the right time to do this.

Before we move forward, I want to establish a very simple naming convention. From the README you can see that the whole tutorial is divided into lessons. Each lesson in its turn consists of individual files that I call "chapters". (Right now, you are reding lesson 1, chapter 1.1) A chapter is further divided into "sections" by headers. This naming convention allows me to make references to different parts of the material.

Another thing I want to pay your attention to is that the tutorial contains a lot of source code samples. Usually, I start the explanation by providing the complete code block. Then I start describing it line by line. I copy a few lines of code and put the description right after them. 

### Project structure

The source code of each lesson has the same structure. You can find this particular lesson source code [here](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson01). Let's briefly describe the main components of this folder.
1. **Makefile** We are using [make utility](http://www.math.tau.ac.il/~danha/courses/software1/make-intro.html) to build the kernel. `make` behavior is configured by a makefile, which contains instructions how to compile and link the source code. 
1. **build.sh or build.bat** You need those files if you want to build the kernel using Docker. In this way, you don't need to have make utility and compiler toolchain installed on your laptop.
1. **src** This folder contains all source code.
1. **include** All header files are placed there. 

### Makefile

Now let's take a closer look at the project makefile. The primary purpose of the make utility is to automatically determine what pieces of a program need to be recompiled, and issue commands to recompile them. If you are not familiar with make and makefiles, I recommend you to read [this](http://opensourceforu.com/2012/06/gnu-make-in-detail-for-beginners/) article. 
Makefile for the first lesson can be found [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/Makefile) The whole makefile is listed below.
```
ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude 

BUILD_DIR = build
SRC_DIR = src

all : kernel7.img

clean :
    rm -rf $(BUILD_DIR) *.img 

$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
    mkdir -p $(@D)
    $(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
    $(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@

C_FILES = $(wildcard src/*.c)
ASM_FILES = $(wildcard src/*.S)
OBJ_FILES = $(C_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%_c.o)
OBJ_FILES += $(ASM_FILES:$(SRC_DIR)/%.S=$(BUILD_DIR)/%_s.o)

DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)

kernel7.img: $(SRC_DIR)/linker.ld $(OBJ_FILES)
    $(ARMGNU)-ld -T src/linker.ld -o $(BUILD_DIR)/kernel7.elf  $(OBJ_FILES)
    $(ARMGNU)-objcopy $(BUILD_DIR)/kernel7.elf -O binary kernel7.img
``` 
Now, let's inspect this file in details.
```
ARMGNU ?= aarch64-linux-gnu
```
The makefile starts with the variable definition. `ARMGNU` is a cross-compiler prefix. We need to use a [cross-compiler](https://en.wikipedia.org/wiki/Cross_compiler) because we are compiling the source code for `arm64` architecture on `x86` machine. So instead of `gcc` we are using `aarch64-linux-gnu-gcc`. 

```
COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude 
```

`COPS` and `ASMOPS` are options that we pass to the compiler when compiling C and assembler code respectively. These options require a short explanation.

* **-Wall** Show all warnings.
* **-nostdlib** Don't use C standard library. Most of the calls in C standard library eventually calls an operating system. We are writing a bare metal program, and we don't have any underlying operating system, so C standard library is not going to work for us anyway.
* **-nostartfiles** Don't use standard startup files. Startup files are responsible for setting an initial stack-pointer, initializing static data, and jumping to the main entry point. We are going to do all of this by ourselves.
* **-ffreestanding** A freestanding environment is one in which the standard library may not exist, and program startup may not necessarily be at "main". The option `-ffreestanding` directs the compiler not to assume that standard functions have their usual definition.
* **-Iinclude** Search for header files in the `include` folder.
* **-mgeneral-regs-only**. Use only general-purpose registers. ARM processors also have [NEON](https://developer.arm.com/technologies/neon) registers. We don't want the compiler to use them because it adds additional complexity (for example, we need to store the registers during a context switch) 

```
BUILD_DIR = build
SRC_DIR = src
```

`SRC_DIR` and `BUILD_DIR` are directories that contain source code and compiled object files respectively.

```
all : kernel7.img

clean :
    rm -rf $(BUILD_DIR) *.img 
```

Next, we define make targets. The first two targets are pretty simple 
 `all` target is the default one, and it is executed whenever you type `make` without any arguments. (`make` always uses the first target as a default one) This target just redirects all work to a different target `kernel7.img`.
`clean` target is responsible for deleting all compilation artifacts and compiled kernel image.

```
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
    mkdir -p $(@D)
    $(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
    $(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@
```

Next two targets are responsible for compiling C and assembler files. If, for example, in the `src` directory we have `foo.c` and `foo.S` files, they will be compiled into `/buid/foo_c.o` and `build/foo_s.o` respectively. `$<` and `$@` are substituted at the runtime with the input and output filenames (`foo.c` and `foo_c.o`) Before compiling C files, we also create `build` directory in case it doesn't exist yet.

```
C_FILES = $(wildcard src/*.c)
ASM_FILES = $(wildcard src/*.S)
OBJ_FILES = $(C_FILES:.c=_c.o)
OBJ_FILES += $(ASM_FILES:.S=_s.o)
```

Here we are building an array of all object files (`OBJ_FILES`), created from the concatenation of both C and assembler source files.

```
DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)
```

Next two lines are a little bit tricky. If you once again take a look at how we defined our compilation targets for both C and assembler source files, you might notice that we used `-MMD` parameter. This parameter instructs `gcc` compiler to create a dependency file for each generated object file. A dependency file is a file in makefile format that defines all dependencies for a particular source file. These dependencies usually contain a list of all included headers. Then we need to include all generated dependency files, so that make knows what exactly to recompile in case when some header changes. 

```
$(ARMGNU)-ld -T src/linker.ld -o kernel7.elf  $(OBJ_FILES)
``` 

Then, we use `OBJ_FILES` array to build `kernel7.elf` file. We use linker script `src/linker.ld`  to define basic layout of the resulting executable image (we will discuss the linker script in the next section).

```
$(ARMGNU)-objcopy kernel7.elf -O binary kernel7.img
```

`kernel7.elf` is in [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) format.  But the problem is that ELF files are designed to be executed by an operating system. To write a bare metal program, we need to extract all executable and data sections from the ELF file and put them into `kernel7.img` image. 

### The linker script

The primary purpose of the linker script is to describe how the sections in the input object files (`_c.o` and `_s.o`) should be mapped into the output file (`.elf`). More information about linker scripts can be found [here](https://sourceware.org/binutils/docs/ld/Scripts.html#Scripts). Now let's take a look at the RPi OS linker script.

```
SECTIONS
{
    .text.boot : { *(.text.boot) }
    .text :  { *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    . = ALIGN(0x8);
    bss_begin = .;
    .bss : { *(.bss*) } 
    bss_end = .;
}
``` 

After startup, Raspberry Pi loads `kernel7.img` into memory and starts execution from the beginning of the file. That's why `.text.boot` section must be first - we are going to put OS startup code inside this section. 
`.text`, `.rodata` and `.data` sections contain kernel compiled instructions, read-only data and normal data - there is nothing special to add about them.
`.bss` section contains data that should be initialized to 0. By putting such data in a separate section compiler can save some space in a elf binary - only section size is stored in the elf header but section itself is omitted. After loading the image into memory, we must initialize `.bss` section to 0, that's why we need to record start and end of the section (`bss_begin` and `bss_end` symbols) and align section so that it starts at the address that is multiple of 8. If the section is not aligned it would be more difficult to use `str` instruction to store 0 at the beginning of the `bss` section because `str` instruction can be used only with 8 byte aligned addresses.

### Booting the kernel

Now it is time to take a look at [boot.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/boot.S) file. This file contains kernel startup code.

```
#include "mm.h"

.section ".text.boot"

.globl _start
_start:
    mrs    x0, mpidr_el1        
    and    x0, x0,#0xFF        // Check processor id
    cbz    x0, master        // Hang for all non-primary CPU
    b    proc_hang

proc_hang: 
    b proc_hang

master:
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     memzero

    mov    sp, #LOW_MEMORY
    bl    kernel_main
```
Next let's review this file in details.
```
.section ".text.boot"
```
First of all, we specify that everything defined in `boot.S` should go to the `.text.boot` section. Previously we saw that this section is placed at the beginning of the kernel image by the linker script. So when the kernel is started execution begins at the `start` function. 
```
.globl _start
_start:
    mrs    x0, mpidr_el1        
    and    x0, x0,#0xFF        // Check processor id
    cbz    x0, master        // Hang for all non-primary CPU
    b    proc_hang
```

The first thing this function does is checking processor id. Raspberry Pi 3 has four core processor, and after the device is powered on each core begins to execute the same code. But we don't want to work with 4 cores; we want to work only with the first one and send all other cores to an endless loop. This is exactly what `_start` function is responsible for. It checks processor ID from [mpidr_el1](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0500g/BABHBJCI.html) system register. 
If current process ID is 0 then execution is transferred to `master` function. 

```
master:
    adr    x0, bss_begin
    adr    x1, bss_end
    sub    x1, x1, x0
    bl     memzero
```

Here we clean `.bss` section by calling `memzero`. We will define this function later. In ARMv8 architecture by convention first 7 arguments are passed to called function via registers x0 - x6. `memzeo` function accepts only 2 arguments: start address (`bss_begin`) and size of the section needed to be cleaned (`bss_end - bss_begin`).

```
    mov    sp, #LOW_MEMORY
    bl    kernel_main
```

After cleaning the `.bss` section we initialize stack pointer and pass execution to `kernel_main` function. Raspberry Pi loads the kernel at address 0, that's why initial stack pointer can be set to any location high enough so that stack will not override the kernel image when grows sufficiently large. `LOW_MEMORY` is defined in [mm.h](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/include/mm.h) and is equal to 4MB. Our kernel's stack can't grow large and the image itself is tiny, so `4MB` is more than enough for us. 

For those of you who are not familiar with arm assembler syntax let me quickly summarize the instructions, that we have used.

* **mrs**  Load value from a system register to one of the general purpose registers (x0 - x30)
* **and** Perform logical AND operation. We use this command to strip the last two bytes from the value we obtain from `mpidr_el1` register.
* **cbz** Compare the result of the previously executed operation to 0 and jump (or `branch` in arm terminology) to the provided label if the comparison yielded true.
* **b** Perform an unconditional branch to some label.
* **lrd** Load the address of some label in a general purpose register. 
* **sub** Subtract values from two registers.
* **bl** "Branch with a link" perform unconditional branch and store return address in x30 register. When called function execute `ret` instruction the value from `x30` register is used to jump back to the original location.
* **mov** Moves some value between registers or from a constant to a register.

### `kernel_main` function

We have seen that boot code eventually passes control to the `kernel_main` function. Let's take a look at it.

```
#include "mini_uart.h"

void kernel_main(void)
{
    uart_init();
    uart_send_string("Hello, world!\r\n");

    while (1) {
        uart_send(uart_recv());
    }
}

```

This function is one of the simplest in the kernel. It works with `Mini UART` device to print something to screen and read user input. The kernel just prints `Hello, world!` message and then enters an infinite loop that reads a character from the user and sends it back to the screen.

### Raspberry Pi devices 

Now is the first time we are going to dig into something specific to Raspberry Pi. Before we begin, I recommend you to download [BCM2835 ARM Peripherals manual](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf) BCM2835 is a board that is used by the Raspberry Pi Model A, B, B+. Raspberry Pi 3 uses the BCM2837 board, but its underlying architecture is identical to BCM2835.

Before we proceed to the implementation details, I want to share some basic concepts on how to work with memory mapped devices. BCM2835 is a simple SOC (System On Chip) board. In such a board, access to all devices is performed via memory mapped registers. Raspberry Pi 3 reserves the memory above the address `0x3F000000`  for devices. To activate or configure a particular device, you need to write some data in one of the device's registers. Device register is just a 32-bit region of memory. The meaning of each bit in each device register is described in the BCM2835 ARM Peripherals manual.

From the `kernel_main` function, you can guess that we are going to work with a Mini UART device. UART stands for [Universal asynchronous receiver-transmitter](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter) This device is capable of converting values stored in one of its memory mapped registers to a sequence of high and low voltage. This sequence is passed to your computer via `TTL to serial cable` and is interpreted by your terminal emulator. We are going to use the Mini UART to organize communication with our Raspberry Pi. If you want to see the specification of all Mini UART registers, please open page 8 of the `BCM2835 ARM Peripherals` manual.

Another device that you need to familiarize yourself with is a GPIO [General-purpose input/output](https://en.wikipedia.org/wiki/General-purpose_input/output) GPIO is responsible for controlling `GPIO pins`. You should be able to easily recognize them in the image below.

![Raspberry Pi GPIO pins](../../images/gpio-pins.jpg)

GPIO can be used to configure the behavior of different GPIO pins. For example, to be able to use the Mini UART, we need to activate pins 14 and 15  and setup them to use this device. The image below illustrates how numbers are assigned to the GPIO pins.

![Raspberry Pi GPIO pin numbers](../../images/gpio-numbers.png)

### Mini UART initialization

Now let's take a look at how mini UART is initialized. This code is defined in [mini_uart.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/mini_uart.c)

```
void uart_init ( void )
{
    unsigned int selector;

    selector = get32(GPFSEL1);
    selector &= ~(7<<12);                   // clean gpio14
    selector |= 2<<12;                      // set alt5 for gpio14
    selector &= ~(7<<15);                   // clean gpio15
    selector |= 2<<15;                      // set alt5 for gpio 15
    put32(GPFSEL1,selector);

    put32(GPPUD,0);
    delay(150);
    put32(GPPUDCLK0,(1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0,0);

    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to it registers)
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
    put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
    put32(AUX_MU_IIR_REG,6);                //Clear FIFO

    put32(AUX_MU_CNTL_REG,3);               //Finaly, enable transmitter and receiver
}
``` 

Here we use two functions `put32` and `get32`. Those functions are very simple - they allow us to read and write some data from and to a 32-bit register. You can take a look at how they are implemented in [utils.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/utils.S).  `uart_init` is one of the most complex and important functions in this lesson, and we will continue to examine it in the next three sections.

#### GPIO alternative function selection 

First of all, we need to activate GPIO pins. Most of the pins can be used with different devices, so, before using a particular pin, we need to select pin `alternative function`. An `alternative function` is just a number from 0 to 5 that can be set for each pin and that configures which device is connected to the pin. You can see the list of all available GPIO alternative functions on the image below (The image is taken from page 102 of `BCM2835 ARM Peripherals` manual)

![Raspberry Pi GPIO alternative functions](../../images/alt.png?raw=true)

Here you can see that pins 14 and 15 have TXD1 and RXD1 alternative functions available. This means that if we select alternative function number 5 for pins 14 and 15, they will be used as a Mini UART Transmit Data pin and Mini UART Receive Data pin correspondingly. `GPFSEL1` register is used to control alternative functions for pins 10-19. The meaning of all bits in that registers is shown in the following table (page 92 of `BCM2835 ARM Peripherals` manual) 

![Raspberry Pi GPIO function selector](../../images/gpfsel1.png?raw=true)

So now you know everything you need to understand the following lines of code, that are used to configure GPIO pins 14 and 15 to work with the Mini UART device.

```
    unsigned int selector;

    selector = get32(GPFSEL1);
    selector &= ~(7<<12);                   // clean gpio14
    selector |= 2<<12;                      // set alt5 for gpio14
    selector &= ~(7<<15);                   // clean gpio15
    selector |= 2<<15;                      // set alt5 for gpio 15
    put32(GPFSEL1,selector);
```

#### GPIO pull-up/down 

When you work with Raspberry Pi GPIO pins, you can often encounter such terms as pull-up/pull-down. Those concepts are explained in great details in [this](https://grantwinney.com/using-pullup-and-pulldown-resistors-on-the-raspberry-pi/) article. For those who are too lazy to read the whole article, I will try to briefly explain the pull-up/pull-down concept.

If you use a particular pin as an input and don't connect anything to this pin you will not be able to identify whether the value of the pin is 1 or 0. In fact, the device will report random values. Pull-up/pull-down mechanism allows you to overcome this issue. If you set the pin to pull-up state and nothing is connected to it, it will report `1` all the time (for pull-down state, on the contrary, the value will always be 0) In our case we need neither pull-up nor pull-down state, because both 14 and 15 pins are going to be connected all the time. The pin state is preserved even after a reboot, so before using any pin we always have to initialize its state. There are 3 available states: pull-up, pull-down and the state were both pull-up and pull-down are removed from the pin, and we need the third one.

Switching between pin states is not a very simple procedure because it requires to physically toggle some switch on the electric circuit. This process involves `GPPUD` and `GPPUDCLK` registers and is described on page 101 of `BCM2835 ARM Peripherals` manual. I am copying this description here.

```
The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
the respective GPIO pins. These registers must be used in conjunction with the GPPUD
register to effect GPIO Pull-up/down changes. The following sequence of events is
required:
1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
to remove the current Pull-up/down)
2. Wait 150 cycles – this provides the required set-up time for the control signal
3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
modify – NOTE only the pads which receive a clock will be modified, all others will
retain their previous state.
4. Wait 150 cycles – this provides the required hold time for the control signal
5. Write to GPPUD to remove the control signal
6. Write to GPPUDCLK0/1 to remove the clock
``` 

This procedure describes how we can remove both pull-up and pull-down states from some pin, and that is what we are doing for pins 14 and 15 in the following code.

```
    put32(GPPUD,0);
    delay(150);
    put32(GPPUDCLK0,(1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0,0);
```

#### Initializing the Mini UART

Now our Mini UART is connected to GPIO pins and the pins are configured. The rest of the `uart_init` function is dedicated to Mini UART initialization. 

```
    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to it registers)
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
    put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
    put32(AUX_MU_IIR_REG,6);                //Clear FIFO

    put32(AUX_MU_CNTL_REG,3);               //Finaly, enable transmitter and receiver
```
Let's examine this code snippet line by line. 

```
    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to it registers)
```
This line enables the Mini UART. We must do this in the beginning, because this also enables access to all other Mini UART registers.

```
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
```
Here we disable receiver and transmitter before the configuration is finished. We also permanently disable auto flow control because it requires us to use additional GPIO pins and TTL to serial cable just don't support it. For more information about auto flow control, you can refer to [this](http://www.deater.net/weave/vmwprod/hardware/pi-rts/) article.

```
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
```
It is possible to configure the Mini UART to generate a processor interrupt each time new data is available. We are going to start working with interrupts only in lesson 3, so for now, we just disable this feature.

```
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
```
Mini UART can support either 7 or 8-bit operations. This is because ASCII character is 7 bits for the standard set and 8 bits for the extended. We are going to use 8-bit mode. The interesting thing is that in the description of the `AUX_MU_LCR_REG` register `BCM2835 ARM Peripherals` manual has an error. All such errors are listed  [here](https://elinux.org/BCM2835_datasheet_errata)

```
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
```
RTS line is used in the flow control and we don't need it. Set it to be high all the time.
```
    put32(AUX_MU_BAUD_REG,270);             //Set baud rate to 115200
```
The baud rate is the rate at which information is transferred in a communication channel. “115200 baud” means that the serial port is capable of transferring a maximum of 115200 bits per second. Baud rate should be the same in your Raspberry Pi mini UART device and in your terminal emulator. 
Mini UART calculates baud rate accordingly to the following equation
```
baudrate = system_clock_freq / (8 * ( baudrate_reg + 1 )) 
```
`system_clock_freq` is 250 MHz, so we can easily calculate the value of `baudrate_reg` as 270.

``` 
    put32(AUX_MU_CNTL_REG,3);               //Finaly, enable transmitter and receiver
```
After this line is executed the Mini UART is ready for work!

### Sending data using the Mini UART

After the Mini UART is ready, we can try to send and receive some data using it. To do this, we can use the following two functions. 

```
void uart_send ( char c )
{
    while(1) {
        if(get32(AUX_MU_LSR_REG)&0x20) 
            break;
    }
    put32(AUX_MU_IO_REG,c);
}

char uart_recv ( void )
{
    while(1) {
        if(get32(AUX_MU_LSR_REG)&0x01) 
            break;
    }
    return(get32(AUX_MU_IO_REG)&0xFF);
}
```

Both of the functions start with an infinite loop, the purpose of which is to verify whether the device is ready to transmit or receive data. We are using  `AUX_MU_LSR_REG` register to do this. The first bit, if set to 1, indicates that data is ready, this means that we can read from the UART, the fifth bit tells us that the transmitter is empty so that we can write to the UART.
Next, we use `AUX_MU_IO_REG` to either store value of the transmitting character or read value of the receiving character.

We also have a very simple function that is capable of sending strings instead of characters.

```
void uart_send_string(char* str)
{
    for (int i = 0; str[i] != '\0'; i ++) {
        uart_send((char)str[i]);
    }
}
```
This function just iterates over all characters in a string and  sends them one by one. 

### Raspberry Pi config

Raspberry Pi startup sequence is the following (simplified):

1. The device is powered on.
1. GPU starts first, it reads `config.txt` file from boot partition. This file contains some configuration parameters that GPU uses to adjust further startup sequence.
1. `kernel7.img` is loaded into memory and executed.

To be able to run our simple OS, `config.txt` file should be the following

```
arm_control=0x200
kernel_old=1
disable_commandline_tags=1
```
* `arm_control=0x200` specifies that processor should be booted in 64 bit mode. 
* `kernel_old=1` tels that kernel image should be loaded at the address 0.
* `disable_commandline_tags` instructs the GPU to don't pass any command line arguments to the booted image.


### Testing the kernel

Now, as we have gone through all source code, it is time to see how it works. To build and test the kernel you need to  do the following:

1. Execute `./build.sh` of `./build.bat` from [src/lesson01](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson01) in order to build the kernel. 
1. Copy generated `kernel7.img` file to `boot` partition of your Raspberry Pi flash card.
1. Modify `config.txt` file as was described in the previous section.
1. Connect USB to TTL serial cable as was described in the [Prerequsities](docs/Prerequisites.md)
1. Power on your Raspberry PI (This can be done using the same USB to TTL serial cable)
1. Open your terminal emulator. You should be able to see `Hello, world!` message there.



