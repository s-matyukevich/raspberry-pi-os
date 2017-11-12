#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq.h"
#include "fork.h"
#include "sched.h"
#include "mini_uart.h"
#include "sys.h"
#include "user.h"


void kernel_process(){
	printf("Kernel process started. EL %d\r\n", GET_EL());
	unsigned long begin = (unsigned long)&user_begin;
	unsigned long end = (unsigned long)&user_end;
	unsigned long process = (unsigned long)&user_process;
	move_to_user_mode(begin, end - begin, process - begin);
}


void kernel_main(unsigned long start)
{
	uart_init();
	init_printf(NULL, putc);
	printf("start\n\r");
	printf("addr: %x\n\r", start >> 32);
	irq_vector_init();
	printf("enabled\n\r");
	timer_init();
	enable_interrupt_controller();
	enable_irq();

	int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0);
	if (res < 0) {
		printf("error while starting kernel process");
		return;
	}

	while (1){
		schedule();
	}	
}
