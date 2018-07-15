#include "printf.h"
#include "utils.h"
#include "timer.h"
#include "irq.h"
#include "fork.h"
#include "sched.h"
#include "mini_uart.h"
#include "mm.h"

void process(char *array)
{
	if(array[0] == '1') {
		set_fpsimd_reg0(1);
		set_fpsimd_reg2(2);
		set_fpsimd_reg31(3);
	} else {
		set_fpsimd_reg0(5);
		set_fpsimd_reg2(6);
		set_fpsimd_reg31(7);
	}

	while (1) {
		for (int i = 0; i < 5; i++){
			unsigned int a = get_fpsimd_reg0();
			unsigned int b = get_fpsimd_reg2();
			unsigned int c = get_fpsimd_reg31();
			uart_send(a + '0');
			delay(100000);
			uart_send(b + '0');
			delay(100000);
			uart_send(c + '0');
			delay(100000);
		}
	}
}

void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
	irq_vector_init();
	timer_init();
	enable_interrupt_controller();
	enable_irq();

	int res = copy_process((unsigned long)&process, (unsigned long)"12345");
	if (res != 0) {
		printf("error while starting process 1");
		return;
	}
	res = copy_process((unsigned long)&process, (unsigned long)"abcde");
	if (res != 0) {
		printf("error while starting process 2");
		return;
	}

	while (1){
		schedule();
	}	
}
