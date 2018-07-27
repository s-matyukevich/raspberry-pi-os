#include "printf.h"
#include "utils.h"
#include "mini_uart.h"


void print_el()
{
 	int el = get_el();
	printf("Exception level: %d \r\n", el); 
}

void kernel_init()
{
	uart_init();
	init_printf(0, putc);
	print_el();
}

void kernel_main(void)
{
	print_el();
	while (1) {
		uart_send(uart_recv());
	}
}