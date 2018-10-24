/*Works with the mini_uart Universal Asynchronous Receiver/Transmitter for
serial communication*/
#include "mini_uart.h"

void kernel_main(void)
{
	//Prints hello world in screen
	uart_init();
	uart_send_string("Hello, world!\r\n");

	//Then for everything send to it, it is printed to the screen
	while (1) {
		uart_send(uart_recv());
	}
}
