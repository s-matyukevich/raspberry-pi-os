#include "printf.h"
#include "timer.h"
#include "irq.h"
#include "uart.h"


void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
	irq_vector_init();
	timer_init();
	enable_interrupt_controller();
	enable_irq();
	
	printf("Ate aqui ok!\r\n");

	while (1){
		uart_send(uart_recv());
	}	
}
