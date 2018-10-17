#include "mini_uart.h"

void kernel_main(void)
{
	uart_init(BAUDRATE_19200);		// the baud rate constants are definide in 'mini_uart.h'
	uart_send_string("Hello, world!\r\n");

	while (1) {
		uart_send(uart_recv());
	}
}
