#include "mini_uart.h"

void kernel_main(unsigned long long processor_id)
{
	if(processor_id == 0)
		uart_init();

	uart_send_string("Hello, from processor");
	uart_send('0'+processor_id);
	uart_send_string("\r\n");

	if(processor_id == 0){
		while (1) {
			uart_send(uart_recv());
		}
	}
	else{
		while(1){}
	}
}
