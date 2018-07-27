#include "printf.h"
#include "utils.h"
#include "mini_uart.h"

//to run in qemu use: qemu-system-aarch64 -M raspi3 -serial null -serial mon:stdio -kernel <path to kernel8.img>
void kernel_main(void)
{
	uart_init();
	init_printf(0, putc);
	int el = get_el();
	printf("Exception level: %d \r\n", el);

	while (1) {
		uart_send(uart_recv());
	}
}
