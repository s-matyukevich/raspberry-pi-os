#include <stddef.h>
#include <stdint.h>

#include "utils.h"
#include "mini_uart.h"

void kernel_main(void)
{
	uart_init();
	uart_send_string("Hello, world!\r\n");

	while (1)
		uart_send(uart_recv());
}
