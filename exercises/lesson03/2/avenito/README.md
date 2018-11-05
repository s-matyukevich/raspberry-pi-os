# Getting information about the hardware

1. Looking at the [BCM2837 ARM Peripherals manual](https://github.com/raspberrypi/documentation/files/1888662/BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf), we figure out how to use the Mini_UART interrupt.

# Steps to setup the receive interrupts

1. Page 12. AUX_MU_IER_REG to enable receive interrupts. Changes in the "uart_init ( void )"
1. Enable uart interrupt on "enable_interrupt_controller()".
1. Page 9. Look at AUX_IRQ Bit 0 for mini UART interrupt pending in "handle_irq(void)".
1. Create interrupt handler: handle_uart_irq().
