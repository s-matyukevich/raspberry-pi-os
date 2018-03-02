## Lesson 1: Introducing RPi OS, or bare metal "Hello, world!"

We are going to start our journey to OS development world by writing a simple bare metal "Hello, world" application. I assume that at this point you have already gone through [Prerequisites](https://github.com/s-matyukevich/raspberry-pi-os/docs/Prerequisites.md) and have everything ready for the work. If not - now is the right time to do this.

### Project structure

All source code samples in this tutorial have the same structure. You can take a look at the sources for this particular lesson [here](https://github.com/s-matyukevich/raspberry-pi-os/src/lesson01/). Let's briefly describe its components

1. *Makefile*. We are using [make utility](http://www.math.tau.ac.il/~danha/courses/software1/make-intro.html) to build our kernel. A makefile contains instructions how to compile and link the sources. More on this in the next section.
1. *build.sh or build.bat*. You need those files if you want to build the kernel using Docker. In this way, you don't need to have make utility and compiler toolchain installed on your laptop.
1. *src*. This folder contains all source code files.
1. *include*. All header files should be placed here. 

### Makefile

Now let's take a closer look at the makefile. The primary purpose of the make utility is to automatically determine which pieces of a program need to be recompiled, and issue commands to recompile them. If you are not familiar with make and makefiles, I recommend you to read [this](ftp://ftp.gnu.org/old-gnu/Manuals/make-3.79.1/html_chapter/make_2.html) article. 
Makefile for the first lesson can be found [here](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/Makefile) It starts with variables definition. 

```
ARMGNU ?= aarch64-linux-gnu

COPS = -Wall -nostdlib -nostartfiles -ffreestanding -Iinclude -mgeneral-regs-only
ASMOPS = -Iinclude 

BUILD_DIR = build
SRC_DIR = src
```
`ARMGNU` is a cross-compiler prefix. We need to use cross-compiler because we are compiling the source code for `arm64` architecture on `x86` machine. So instead of `gcc` we are using `aarch64-linux-gnu-gcc`. 

`COPS` and `ASMOPS` are options that we pass to the compiler when compiling C and assembler code respectively. Those options require a short explanation.

* *-Wall*. Show all warnings.
* *-nostdlib*. Don't use C standard library. Most of the calls in C standard library eventually calls underlying operating system. We are writing a bare metal program, and we don't have any underlying operating system, so C standard library is not going to work for us.
* *-nostartfiles*. Don't use standard startup files. Startup files are required to at least set an initial stack-pointer, initialize static data, and jump to the main entry point. We are going to do all of this by ourselves.
* *-ffreestanding*. By default, GCC will act as the compiler for a hosted implementation, and presuming that when the names of ISO C functions are used, they have the semantics defined in the standard library. When using this option compiler will not make assumptions about the meanings of function names from the standard library.
* *-Iinclude*. Search for header files in the `include` folder.
* *-mgeneral-regs-only*. Use only general registers. ARM processors also have [NEON](https://developer.arm.com/technologies/neon) registers. We don't want compiler to use them because it adds additional complexity (for example, we need to store the registers during context switch) 

`SRC_DIR` and `BUILD_DIR` are directories that contain source code and compiled object files respectively.

Next, we define make targets. The first two targets are pretty simple 

```
all : kernel7.img

clean :
    rm -rf $(BUILD_DIR) *.img 
```

`all` target is the default, and it is executed whenever you type `make` without any arguments. It just redirects all work to a different target `kernel7.img`
`clean` target is responsible for deleting all compilation artifacts and compiled kernel image.

Next two targets are responsible for compiling C and assembler files.

```
$(BUILD_DIR)/%_c.o: $(SRC_DIR)/%.c
    mkdir -p $(@D)
    $(ARMGNU)-gcc $(COPS) -MMD -c $< -o $@

$(BUILD_DIR)/%_s.o: $(SRC_DIR)/%.S
    $(ARMGNU)-gcc $(ASMOPS) -MMD -c $< -o $@
```

If, for example, in the `src` directory we have `foo.c` and `foo.S` file, they will be compiled into `/buid/foo_c.o` and `build/foo_s.o` respectively by executing `$(ARMGNU)-gcc` compiler with needed options. `$<` and `$@` are substituted in the runtime with the input and output filenames (`foo.c` and `foo_c.o`) Before compiling C files, we also create `build` directory in case it doesn't exist.

Finally, we need to define  `kernel7.img` target. We call our target `kernel7.img` because this is the name if the file that Raspberry Pi attempts to load and execute. But before we can do this, we need to build an array of all object files, created from both C and assembler source files

```
C_FILES = $(wildcard src/*.c)
ASM_FILES = $(wildcard src/*.S)
OBJ_FILES = $(C_FILES:.c=_c.o)
OBJ_FILES += $(ASM_FILES:.S=_s.o)
```

Next two lines are a little bit tricky. If you once again take a look at how we defined our compilation targets for both C and assembler source files, you might notice that we used `-MMD` parameter. This parameter instructs `gcc` compiler to create a dependency file for each generated object file. A dependency file is a file in Makefile format that defines all dependencies of a particular source file. Those dependencies usually contain a list of all included headers. Then we need to include all generated dependency files, so that make knows what exactly to recompile in the case when some header changes. That is what we are doing in the following two lines.

```
DEP_FILES = $(OBJ_FILES:%.o=%.d)
-include $(DEP_FILES)
```

Next, we use `OBJ_FILES` array to build `kernel7.elf` file.

```
$(ARMGNU)-ld -T src/linker.ld -o kernel7.elf  $(OBJ_FILES)
``` 

`kernel7.elf` is in [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format) format. We use linker script `src/linker.ld`  to define basic layout of the resulting executable image (we will discuss linker scripts in the next section). But the problem is that ELF files are designed to be executed by an operating system. To write a bare metal program, we need to extract all executable and data section from ELF file and put them to `kernel7.img` image. That is exactly what you can see in the next line.

```
$(ARMGNU)-objcopy kernel7.elf -O binary kernel7.img
```

So the resulting `kernel7.img` target looks like the following

```
kernel7.img: src/linker.ld $(OBJ_FILES)
    $(ARMGNU)-ld -T src/linker.ld -o kernel7.elf  $(OBJ_FILES)
    $(ARMGNU)-objcopy kernel7.elf -O binary kernel7.img
```

### Linker script

The primary purpose of the linker script is to describe how the sections in the input files (`_c.o` and `_s.o`) should be mapped into the output file (`.elf`). More information about linker scripts can be found [here](https://sourceware.org/binutils/docs/ld/Scripts.html#Scripts). Now let's take a look at the linker script.

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

After startup, Raspberry Pi will load `kernel7.img` into memory and start execution from the beginning of the file. That's why `.text.boot` section must be first. This section we are going to put low-level OS startup code. 
`.text`, `.rodata` and `.data` sections contain kernel compiled instructions, read-only data and normal data - there is nothing special to add about them.
`.bss` section is responsible for containing data that should be initialized to 0. By puting such data in a separate section compiler can save some space in elf binary - only section size is stored in elf header but section itself is omited. After loading image into memory we should initialize `.bss` section to 0, That's why we need to record start and end of the section (`bss_begin` and `bss_end` symbols store this information) and align section so it starts at the address that is multiple of 8. If section is not aligned it would be more difficult to use `str` instruction to store 0 at the beggining of the bss section, because `str` instruction can only be used with 8 byte aligned address.

### Booting the kernel

Now it is time to take a look on [boot.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/boot.S) file. This file contains kernel startup code.

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

First of all, we define that everything defined in this file should go to `.text.boot` section. Previously we saw that this section is placed at the beggining of kernel image by the linker script. So whenever kernel is started execution begins at the `start` function. The first thing this functions does is checking processor id. Raspberry Pi 3 has 4 core processor, and after the device is powered on each core beging to execute the same code. But we don't want to work with 4 cores, we want to work only with the first one and send all other cores to endless loop. This is exactly what `_start` function is responsible for. It checks processor ID from [mpidr_el1](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0500g/BABHBJCI.html) system register. 
If current processon ID is 0 then execution is transfered to `master` function. Here we clean `.bss` section by calling `memzero` function. We will define this function later. In ARMv8 architecture by convention first 7 arguments are passed to called function viar registers x0 - x6. `memzeor` function accept only 2 arguments: start address (`bss_begin`) and size of the section needed to be cleaned (`bss_end - bss_begin`).

After cleanining bss we initialize stack pointer and pass execution to `kernel_main` funnction. Pasberry Pi loads the kernel at address 0, thats why initial stack pointer can be set to any location large enough so that stack will not override kernel image when grows suficiently large. `LOW_MEMORY` is defined in [mm.h](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/include/mm.h) and is equal to 4MB Stack for our simple kernel can't grow large and image itself is very small so `4MB` is more then enough  for us. 

For those of you who are not familiar with arm assembler syntax let me quickly summarize what instaructions, that we have used, are actually doing.

* `mrs` - load value from a system register to one of the general purpose registers (x0 - x30)
* `and` - perform logical AND operation. We use this command to strip last two bytes from the value we obtain from `mpidr_el1` register.
* `cbz` - compare result of previously executed operation to 0 and jump (or `branch` in arm terminology) to `master` label is comparison yeilded true.
* `b` - perform unconditional branch to some label
* `ard` - load in a general purpose register addres of some lable. In our case this label is defined in the linker script.
* `sub` - substract values from two registers.
* `bl` - 'branch with link' perform unconditional branch and store return address in x30 register. Whenever called function execute `ret` instruction value from `x30` register is used to jump back to the original location.
* `mov` - moves some value between registers of from a constant to a register.

### Main kernel function

We have seen that boot code eventually passes control to `kernel_main` function. Let's take a look on it.

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

This function is one of the simplest in the kernel. It works with `Mini UART` device to print something to the screen and read user input (more about this device in the upcomming section). Kernel just prints `Hello, world!` message and then enters infinite loop that read a character from user and sent it bach to the screen.

### Raspberry Pi devices 

Now is the first time we are going to dig into something specific to Raspberry Pi. Before we begin I recommend you to download [BCM2835 ARM Peripherals manual](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf) BCM2835 is a board that is used by Raspberry Pi Model A, B, B+. Raspberry Pi 3 actualy uses BCM2837 board, but it underlying architecture is identical to BCM2835.

Before we proceed to the implementation details I want to share some basic concepts on how to work with memory mapped devices. BCM2835 is a simple SOC(System On Chip) board. Access to all devices is performed via memory mapped registers. Memory above the address 0x3F000000 is reserver for devices and can't be used as a general purpose memory. In order to activate  or configure particular device you need to write some data in one of the device registers. Device register is just a 32 bit region of memory each bit of which has specific meaning. The meaning of each bit in each device register is describe in BCM2835 ARM Peripherals manual.

At this point you may be wondering what is mini UART. UART is [Universal asynchronous receiver-transmitter](https://en.wikipedia.org/wiki/Universal_asynchronous_receiver-transmitter) This deice is cappable of converting values in one of its memory mapped register to a sequence of high and low votage. This sequence is passed to your computer through `TTL to serial cable` and interpreted by your terminal emulator. We are going to use mini UART to organize communication with our Raspberry Pi. If you want to see specification of all mini UART registers, plase  open page 8 of `BCM2835 ARM Peripherals` manual.

Another device that you need to familiarize yourself with is GPIO [General-purpose input/output](https://en.wikipedia.org/wiki/General-purpose_input/output) GPIO is responsible for controlling `GPIO pins`. You should be able to easily recognize them on the image below.

![Raspberry Pi GPIO pins](https://www.raspberrypi.org/documentation/usage/gpio-plus-and-raspi2/images/gpio-pins-pi2.jpg)

GPIO can be used to configure behaviour of different GIPIO pins. For example, in order to be able to use mini UART we need to activate pins 14 and 15  and connect them to mini UART (more about it later) 

![Raspberry Pi GPIO pins](https://www.raspberrypi.org/documentation/usage/gpio-plus-and-raspi2/images/gpio-numbers-pi2.png)

### Mini UART initialization

Now let's take a look on how mini UART is initialized. This code is defined in [mini_uart.c](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/mini_uart.c)

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
    put32(AUX_MU_BAUD_REG,270);             //Set baund rate to 115200
    put32(AUX_MU_IIR_REG,6);                //Clear FIFO

    put32(AUX_MU_CNTL_REG,3);               //Finaly, enable transmitter and receiver
}
``` 

Here we use two functions `put32` and `get32`. Those functions are very simple - they allow us to read and write some date from and to any 32 bit register.You can take a look on how they are implemented in [utils.S](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/src/lesson01/src/utils.S).  

#### GPIO alternative function selection 

First of all, we need to activate GPIO pins. Most of the pins can be used with different devices, so before using particular pin we need to select pin `alternative function`. `alternative function` is just a number from 0 to 5 that can be set for each pin and that can indicate what device is connected to the pin. You can see list of all available GPIO alternative function for raspberry PI on the image below (taken from page 102 of `BCM2835 ARM Peripherals` manual)

![Raspberry Pi GPIO alternative functions](https://github.com/s-matyukevich/raspberry-pi-os/images/alt.png)

Here you can see that pins 14 and 15 have TXD1 and RXD1 alternative functions. This means that if we select 5th alternative function for pins 14 and 15 they would be used as mini UART Transmit Data pin and mini UART Receive Data pin corespondingly. `GPFSEL1` register is used to control alternative functions for pins 10-19. Meaning of the bits in that registers are shown in the following table (page 92 of `BCM2835 ARM Peripherals` manual) 

![Raspberry Pi GPIO function selector](https://github.com/s-matyukevich/raspberry-pi-os/images/gpfsel1.png)

So now you know everything you need to understand the following lines of code, that are used to configure GPIO pins 14 and 15 to work with mini UART device.

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

When you work with Raspberry Pi GPIO you can ofen encounter such terms as  pull-up/pull-down. Those concepts are explain in grat details in [this](https://grantwinney.com/using-pullup-and-pulldown-resistors-on-the-raspberry-pi/) article. For those who are too lazy to read the whole article I will try to brefly explain this concept.

If you use particular pin as an input and don't connect anything to this pin you will not be able to identify whether value of the pin is 1 or 0. In fact device will report random values. Pull-up/pull-down mechanizm allow you to overcome this issue. If you use pull-up and nothin is connected to your pin it will report 1 all the time (for pull-down state value will always be 0) In our case we don't needther pull-up nor pull-down state, because bosh 14 and 15 pins are going to be connected all the time. 
Swithching between pull-up and pull-down  states is not a very simple procedure, because it requires to physically toggle somw switch in the electric circit. The procedure uses `GPPUD` and `GPPUDCLK` registers and is described in page 101 of `BCM2835 ARM Peripherals` manual. I am copying this description here.

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

This procedure desribes how we can remove bosh pull-up and pull-down states from pins 14 and 15, and that is exactly what we are doing in the following lines of code.

```
    put32(GPPUD,0);
    delay(150);
    put32(GPPUDCLK0,(1<<14)|(1<<15));
    delay(150);
    put32(GPPUDCLK0,0);
```

#### Initializing mini UART

Now our mini UART is connected to GPIO pins and those pins are configured. the rest of the function is dedicated to mini UART initialization. Let's examine it line by line. For more information about specific mini UART register, please refer to `BCM2835 ARM Peripherals` manual.

```
    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to it registers)
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
    put32(AUX_MU_BAUD_REG,270);             //Set baund rate to 115200
    put32(AUX_MU_IIR_REG,6);                //Clear FIFO

    put32(AUX_MU_CNTL_REG,3);               //Finaly, enable transmitter and receiver
```

```
    put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to it registers)
```
This line enables mini UART. We must to this in the beggining, because this also enables access to all othe mini UART registers.

```
    put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
```
We disabling reciver and transmitter lines before configuration is finished. We also permanently disable auto flow control because it requires us to use additional GPIO pins and TTL to serial cable simply just don't suport it. For more information about auto flow control you can refer to [this](http://www.deater.net/weave/vmwprod/hardware/pi-rts/) article.

```
    put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
```
It is posible to confugre mini UART to generate a processor interrupt each time new data is vailable. We are going to start working with interrupts only in lesson 3, so for now we just disable this feature.

```
    put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
```
Mini UART can support either 7 or 8 bit operations. This is because ASCII character is 7 bits for the standard set and 8 bits for the extended. We are going to use 8 bit mode. Interesting thing is that in description of AUX_MU_LCR_REG register `BCM2835 ARM Peripherals` manual have an error. All such errors are described  [here](https://elinux.org/BCM2835_datasheet_errata)

```
    put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
```
The baud rate is the rate at which information is transferred in a communication channel. In the serial port context, “115200 baud” means that the serial port is capable of transferring a maximum of 115200 bits per second. Baud rate should be configured the same in your Raspberry Pi mini UART device and in your terminal emulator. 
In raspberry Pi baud rate is calculated accordingly to the following equation
```
baudrate = system_clock_freq / (8 * ( baudrate_reg + 1 )) 
```
`system_clock_freq` is 250 MHz, so we can easily calculate value of `baudrate_reg` as 270.

``` 
    put32(AUX_MU_CNTL_REG,3);               //Finaly, enable transmitter and receiver
```
After this line is executed mini UART device is ready to work!

### Sending data using mini UART

After mini UART is ready we can try to send and recive some data using it. In order to do this we can use the following two functions. 

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

Both of the functions starts with infinete loop the purpose of wichis to verify whether the device is ready. We are using  `AUX_MU_LSR_REG` register to do this. First bit indicate that data is ready, this mean that we can read from UART, fifth bit indicates that transmitter is empty, so we can write to UART.
Next we use `AUX_MU_IO_REG` to either store value of the transmiting character or read value of the receiving character.

We also have a very simple sunction that is cappable of sending strings instead of characters.

```
void uart_send_string(char* str)
{
    for (int i = 0; str[i] != '\0'; i ++) {
        uart_send((char)str[i]);
    }
}
```
This function just iterates over all characters in the string and  send each character one by one. 

### Raspberry Pi config

The simplified startup sequence of raspberry Pi is the following:

1. Device is powered on.
1. GPU starts first, it reads `config.txt` file from boot partition. This file contains some configuration parameters that GPU uses to adjust futher startup sequence.
1. kernel7.img` file is loaded into memory and executed.

In order to be able to run our simple OS `config.txt` file should be the following

```
arm_control=0x200
kernel_old=1
```
`arm_control=0x200` specifies that processor should be booted in 64 bit mode. 
`kernel_old=1` tels that kernel image should be loaded at the address 0.


### Testing the kernel

Now as we have gone throug all source code in the kernel it is time to see how it works. In order to build and test the kernel you need to  do the following steps:

1. Execute `./build.sh` of `./build.bat` from [src/lesson01](https://github.com/s-matyukevich/raspberry-pi-os/tree/master/src/lesson01) in order to build the kernel. 
1. Copy generated `kernel7.img` file to your Raspberry Pi flash card.
1. Modify `config.txt` file as was desribed in previous section.
1. Connect USB to TTL serial cabel as was described in [Prerequsities](https://github.com/s-matyukevich/raspberry-pi-os/blob/master/docs/Prerequisites.md)
1. Power on your Raspberry PI (this can be done using the same USB to TTL serial cabel)
1. Open your terminal emulator. You should be able to see `Hello, world!` message there.

