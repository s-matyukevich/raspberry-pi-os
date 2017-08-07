#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq.h"
#include "mini_uart.h"


void kernel_main(void)
{
	uart_init();
	init_printf(NULL, putc);
	irq_vector_init();
	timer_init();
	enable_interrupt_controller();
	enable_processor_interrupts();

	while (1){
		uart_send(uart_recv());
	}	
}
