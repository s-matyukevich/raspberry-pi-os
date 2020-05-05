#include "mini_uart.h"

void kernel_main(void)
{
	const unsigned int baud_rate = 460800;
	uart_init(baud_rate);
	uart_send_string("Hello, world!\r\n");

	while (1) {
		uart_send(uart_recv());
	}
}
