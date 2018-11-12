#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq.h"
#include "fork.h"
#include "sched.h"
#include "uart.h"

void process(char *array)
{
	printf("Entrou no processo ...\n\r");
	while (1){
		for (int i = 0; i < 5; i++){
			uart_send(array[i]);
			delay(100000);
		}
	}
}

void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
	irq_vector_init();
	generic_timer_init();
	enable_interrupt_controller();
	enable_irq();
	
	printf("Copy of proccess 1\n\r"); //###

	int res = copy_process((unsigned long)&process, (unsigned long)"12345");
	if (res != 0) {
		printf("error while starting process 1");
		return;
	}

	printf("\n\rCopy of proccess 2\n\r"); //###

	res = copy_process((unsigned long)&process, (unsigned long)"abcde");
	if (res != 0) {
		printf("error while starting process 2");
		return;
	}

	printf("\n\rStarting schedule ...\n\r"); //###

	while (1){
		schedule();
	}	
}
