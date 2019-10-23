#include "mini_uart.h"

void kernel_main(void)
{
	uart_init();
	delay(1000);

	/* Explain the function of kernal */
	// FIXME: Show string is mix code, Why? 
	uart_send_string("Show you what you input!\r\n");
	while (1) {
		uart_send(uart_recv());
	}
}
