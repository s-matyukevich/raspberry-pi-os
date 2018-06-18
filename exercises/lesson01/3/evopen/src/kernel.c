#include "mini_uart.h"
#include "utils.h"

void kernel_main(unsigned long id) {
    if (id == 0) {
        uart_init();
    }
    uart_send_string("Hello, from processor ");
    uart_send(id + 48);
    uart_send_string(".\r\n");

    while (1)
        ;
}
