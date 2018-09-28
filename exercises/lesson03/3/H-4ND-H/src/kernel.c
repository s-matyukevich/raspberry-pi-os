#include "printf.h"
#include "timer.h"
#include "irq.h"
#include "mini_uart.h"

//to run in qemu use: qemu-system-aarch64 -M raspi3 -serial null -serial mon:stdio -kernel <path to kernel8.img>
void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
	irq_vector_init();
	timer_init();
	enable_interrupt_controller();
	enable_irq();

	while (1){
		uart_send(uart_recv());
	}	
}
