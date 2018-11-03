#include "pl011_uart.h"

static int semaphore = 0;

void kernel_main(unsigned int proc_id)
{
    if (proc_id == 0)
    {
        pl011_uart_init();
    }

    while (semaphore != proc_id)
    {
        ;
    }

    // semaphore = 1;
    pl011_uart_send_string("Hello, world from processor ");
    pl011_uart_send((char)(proc_id + '0'));
    pl011_uart_send_string("\r\n");
    semaphore++;

    if (proc_id == 0)
    {
        while (1)
        {
            pl011_uart_send(pl011_uart_recv());
        }
    }
}
