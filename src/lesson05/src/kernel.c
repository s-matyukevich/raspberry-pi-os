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

void user_process1(char *array)
{
	char buf[2] = {0};
	while (1){
		for (int i = 0; i < 5; i++){
			buf[0] = array[i];
			call_sys_write(buf);
			DELAY(100000);
		}
	}
}

void user_process(){
	char buf[30] = {0};
	tfp_sprintf(buf, "User process started\n\r");
	call_sys_write(buf);
	unsigned long stack = call_sys_malloc();
	int err = call_sys_clone((unsigned long)&user_process1, (unsigned long)"12345", stack);
	if (err < 0){
		tfp_sprintf(buf, "Error in clone 1\n\r");
		return;
	} 
	stack = call_sys_malloc();
	err = call_sys_clone((unsigned long)&user_process1, (unsigned long)"abcd", stack);
	if (err < 0){
		tfp_sprintf(buf, "Error in clone 2\n\r");
		return;
	} 
	call_sys_exit();
}

void kernel_process(){
	printf("Kernel process started. EL %d\r\n", GET_EL());
	move_to_user_mode((unsigned long)&user_process);
}


void kernel_main(void)
{
	uart_init();
	init_printf(NULL, putc);
	printf("start");
	irq_vector_init();
	timer_init();
	enable_interrupt_controller();
	enable_processor_interrupts();

	int res = copy_process(PF_KTHREAD, (unsigned long)&kernel_process, 0, 0);
	if (res < 0) {
		printf("error while starting kernel process");
		return;
	}

	while (1){
		schedule();
	}	
}
