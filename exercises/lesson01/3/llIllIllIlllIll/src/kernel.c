#include "mini_uart.h"

void kernel_main(unsigned long long processor_id)
{
	static unsigned long long current_speaking_id = 0;

	if(processor_id == 0)
		uart_init();

	while(processor_id != current_speaking_id)
		;
	uart_send_string("Hello, from processor");
	uart_send('0'+processor_id);
	uart_send_string("\r\n");

	current_speaking_id++;

	if(processor_id == 0){
		while (1) {
			uart_send(uart_recv());
		}
	}
	else{
		while(1){}
	}
}
