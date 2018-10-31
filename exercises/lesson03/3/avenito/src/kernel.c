#include "printf.h"
#include "timer.h"
#include "irq.h"
#include "uart.h"
#include "entry.h"
#include "peripherals/irq.h"
#include "peripherals/timer.h"

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
		//uart_send(uart_recv());
		uart_recv();
		printf("ENABLE_IRQS_1: %04x \n\r", get32(ENABLE_IRQS_1));
		printf("IRQ_PENDING_1: %04x \n\r", get32(IRQ_PENDING_1));
		printf("TIMER_CLO: %04x \n\r", get32(TIMER_CLO));
		printf("TIMER_C1: %d \n\r", get32(TIMER_C1));
	}	
}
