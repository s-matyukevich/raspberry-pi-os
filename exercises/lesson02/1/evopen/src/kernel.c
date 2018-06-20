#include "mini_uart.h"
#include "printf.h"
#include "utils.h"

void kernel_main(void) {
    uart_init();
    init_printf(0, putc);
    int el = get_el();
    printf("Exception level: %d \r\n", el);
}
