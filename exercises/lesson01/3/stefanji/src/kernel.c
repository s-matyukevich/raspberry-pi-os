#include "mini_uart.h"
#include "utils.h"

void kernel_main(unsigned int core_id)
{
	if(core_id == 0){ uart_init(); }
	else { delay(300000 * core_id); } //TODO find a better way

	uart_send_string("Hello Word From #");
	uart_send(core_id + '0');
	uart_send_string(" Processor Core.\r\n");
	
	if(core_id == 0) while (1) {
		uart_send(uart_recv());
	}
	else while(1) {};
}
