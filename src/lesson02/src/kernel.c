#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "mini_uart.h"

void kernel_main(void)
{
	uart_init();
	init_printf(NULL, putc);
	int el = GET_EL();
	printf("Exception level: %d \r\n", el);

	while (1)
		uart_send(uart_recv()); 
}
