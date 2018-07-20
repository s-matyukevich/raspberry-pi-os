#include "mini_uart.h"


//To run in qemu use "qemu-system-aarch64 -M raspi3 -serial null -serial mon:stdio -kernel <kernel8.img address>"
void kernel_main(void)
{
	uart_init();
	uart_send_string("Hello, world!\r\n");

	while (1) {
		uart_send(uart_recv());
	}
}
