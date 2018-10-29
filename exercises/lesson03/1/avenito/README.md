# Getting information about the hardware

1. Looking at the issue suggested [here] (https://github.com/s-matyukevich/raspberry-pi-os/issues/70), we can find out by [BCM2836 ARM-local peripherals](https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf) how to use the peripherals.

1. BCM2836 ARM-local peripherals, page 3, has the ARM address map and we can verify the base address of Local peripherals is 0x40000000.

1. Page 7 we find the register's addresses for "Local timer".
0x4000_0034 Local timer control & status
0x4000_0038 Local timer write flags

1. Page 17 bits of Local timer control & status, Page 18 bits of Local timer write flags


