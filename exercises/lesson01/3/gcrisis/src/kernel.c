#include "mini_uart.h"
#include "utils.h"
#include "peripherals/mini_uart.h"
#include<stdbool.h>

bool is_uart_inited=false;
void kernel_main(int corenum)
{
    if(0==corenum)
    {
        uart_init(115200);
        is_uart_inited = true;
        //output something to make sure only init uart once
        uart_send(corenum+'0');
        uart_send_string("\r\n");
    }
    while(!is_uart_inited);    //wait uart init OK
    delay(corenum*200000);     //I find the origin delay cannot use 0 as parameter, so I make some change to delay function 
    uart_send_string("Hello, world! From processor <");
    uart_send(corenum+'0');
    uart_send_string(">\r\n");

    if(0==corenum)
    {
        while (1) {
            uart_send(uart_recv());
        }

    }
    else
    {
        while(1);
    }
}
