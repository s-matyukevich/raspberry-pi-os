#include "mini_uart.h"
#include "printf.h"
#include "utils.h"

void kernel1_main(void)
{
    //init uart and printf
	uart_init(115200);
    init_printf(0,putc);

	while (1) {
        printf("current EL:%d\r\n",get_el());
        uart_send(uart_recv());
	}
}

void kernel2_main(void)
{
    //init uart and printf
	uart_init(115200);
    init_printf(0,putc);
    printf("current EL:%d\r\n",get_el());
    return;
}
