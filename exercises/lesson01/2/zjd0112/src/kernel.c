#include "pl011_uart.h"

void kernel_main(void)
{
	pl011_uart_init();
	pl011_uart_send_string("Hello, world!\r\n");

	while (1) {
		pl011_uart_send(pl011_uart_recv());
	}
}
