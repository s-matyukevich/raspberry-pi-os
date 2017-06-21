#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq_c.h"
#include "irq_s.h"
#include "mini_uart.h"


void kernel_main(void)
{
	uart_init();
	uart_send('1');
	init_printf(NULL, putc);
	int el = GET_EL();
	printf("Exception level: %d \r\n", el);
	irq_vector_init();
	timer_init();
	enable_interrupt_controller();
	enable_processor_interrupts();

	/*  int t = 0;
	while (t < 1000000){
		t++;
	} */
	printf("loop ended \n\r");

	while (1){
		char c = uart_recv();
		uart_send(c);
	}	
}
