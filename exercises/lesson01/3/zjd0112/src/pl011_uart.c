#include "utils.h"
#include "peripherals/pl011_uart.h"
#include "peripherals/gpio.h"

void pl011_uart_send(char ch)
{
    /*Transmit FIFO is full*/
    while (get32(UART_FR) & (1<<5))
    {
        ;
    }
    put32(UART_DR, ch);
}

char pl011_uart_recv(void)
{
    /*Receive FIFO is empty*/
    while (get32(UART_FR) & (1<<4))
    {
        ;
    }
    return (get32(UART_DR) & 0xFF);
}

void pl011_uart_send_string(char* str)
{
    for (int i = 0; str[i] != '\0'; i++)
    {
        pl011_uart_send((char)str[i]);
    }
}

void pl011_uart_init(void)
{
    unsigned int selector;

    /*set gpio function*/
    selector = get32(GPFSEL1);
    selector = selector & (~(7<<12));   // clean gpio14
    selector = selector | (4<<12);      // set alt1 for gpio14 
    selector = selector & (~(7<<15));   // clean gpio15
    selector = selector | (4<<15);      // set alt1 for gpio15
    put32(GPFSEL1, selector);

    /*set gpio pull-up/down*/
    put32(GPPUD, 0);
    delay(150);
    put32(GPPUDCLK0, (1<<14 | 1<<15));
    delay(150);
    put32(GPPUDCLK0, 0);

    /*Initializing PL011 uart*/
    put32(UART_CR, 0);                      // disable uart
    put32(UART_IBRD, 26);                   // set baudrate = 115200
    put32(UART_FBRD, 3);
    put32(UART_LCRH, (1<<4) | (3<<5));      // 8bits and enable FIFO
    put32(UART_CR, (1 | (3<<8)));           // enable uart, rx, tx
}
