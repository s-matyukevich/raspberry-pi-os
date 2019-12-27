#include "mini_uart.h"

void kernel_main(void)
{
    char rcv;
	uart_init(BAUD_RATE);
	uart_send_string("\rHello, world!\r\n");

	while (1) {
        rcv = uart_recv();
		uart_send(rcv);
        if ('\r' == rcv)
            uart_send('\n');
	}
}
