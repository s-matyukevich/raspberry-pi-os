#include "mini_uart.h"

void kernel_main(void)
{
	uart_init(BAUD_9600);
	uart_send_string("Hello, world!\r\n");

	while (1) {
		uart_send(uart_recv());
	}
}
