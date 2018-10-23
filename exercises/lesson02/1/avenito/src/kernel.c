#include "printf.h"
#include "utils.h"
#include "mini_uart.h"

void kernel_main_first(void)
{
	uart_init();
	init_printf(0, putc);
	int el = get_el();
	printf("1.Exception level: %d \r\n", el);
	return;
}

void kernel_main_second(void)
{
	int el = get_el();
	printf("2.Exception level: %d \r\n", el);

	while (1) {
		uart_send(uart_recv());
	}
}
