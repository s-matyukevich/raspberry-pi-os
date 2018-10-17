#include "utils.h"
#include "mm.h"
#include "mini_uart.h"

static unsigned int processor = 0;	// global variable to sinc the processors

void kernel_main(char proc_id)		// get processor id as 'char' from 'x0'
{

	while (processor != proc_id) {}	// wait to execute

	if (proc_id == 0) {				// only the master (processor id = 0) initialize uart

		uart_init();

	}

	uart_send_string("Hello, world! I am the Core ");
	uart_send(proc_id + '0');		// add '0' (0x30) to converte to ascii
	uart_send_string("\r\n");
	processor++;					// increment 'processor' to enable the next core to execute

	if (proc_id == 0) {

		while (1) {

			uart_send(uart_recv());	// only the master echos the typed character through uart

		}

	} else {

		while (1) {}				// all the other cores in loop here

	}

}

