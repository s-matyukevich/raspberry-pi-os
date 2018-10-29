# Getting information about the hardware

1. Looking at the suggested issue [here](https://github.com/s-matyukevich/raspberry-pi-os/issues/70), we can find out by [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) how to use the peripherals.

1. BCM2836 ARM-local peripherals, page 3, has the ARM address map and we can verify the base address of Local peripherals is 0x40000000.

1. On page 7 we find the register's addresses for "Local timer".  
0x4000_0034 Local timer control & status  
0x4000_0038 Local timer write flags

1. On page 17, bits of Local timer control & status, Page 18 bits of Local timer write flags.

# Steps to setup the "Local Timer"

1. Following the "kernel.c" ...  
1.1 timer_init() - reload the time interval - Page 17 - Address: 0x4000_0034 Bits 0:27. According to the information, the value to generate an interruption each 1 sec can be calculated by the equation "Interval(Hz) = 38.4*10^6 / value", but it doesn't work for me, then, I simply got an "aleatory" value. Enable Timer (1<<28).  
1.1 enable_interrupt_controller() - it isn't enabling interrupt controller, instead, it's enabling local timer interrupt.  
1.1 enable_irq() - nothing to change.  
1.1 handle_timer_irq() - clear the interrupt flag and reload timer at 0x4000_0038.  