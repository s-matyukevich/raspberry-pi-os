#include "printf.h"
#include "timer.h"
#include "irq.h"
#include "uart.h"
#include "entry.h"
#include "utils.h"
#include "peripherals/irq.h"
#include "peripherals/timer.h"

void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
	irq_vector_init();
	generic_timer_init();
	enable_interrupt_controller();
	enable_irq();
	
	while (1){
		//uart_send(uart_recv());
		uart_recv();
		printf("IRQ_PENDING_1: %04x \n\r", get32(IRQ_PENDING_1));
	}	
}
