#include "utils.h"
#include "peripherals/uart.h"
#include "peripherals/gpio.h"

void uart_send ( char c )
{
    while(1)
    {
        if(get32(UART_FR) & (1<<7)) 
        {
            break;
        }

    }
	put32(UART_DR,c);
}

char uart_recv ( void )
{
    while(1)
    {
        if(get32(UART_FR) & (1<<6))
        {
            break;
        }

    }
	return(get32(UART_DR)&0xFF);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

void uart_init (int baudrate)
{
	unsigned int selector;

	selector = get32(GPFSEL1);
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 4<<12;                      // set alt0 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 4<<15;                      // set alt0 for gpio15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

    put32(UART_CR,0);
    //need remove -mgeneral-regs-only in Makefile
    int ibrd = 3000000/baudrate;
    int fbrd = (int)((3000000.0f/baudrate -ibrd)*64.f + 0.5);
    //LCRH must be set behind IBRD and FBRD,please refer to page 3-14 in http://infocenter.arm.com/help/topic/com.arm.doc.ddi0183g/DDI0183G_uart_pl011_r1p5_trm.pdf
	put32(UART_IBRD,ibrd);             //Set baud rate integer part
	put32(UART_FBRD,fbrd);             //Set baud rate frational part
	put32(UART_LCRH,(3<<5));             //Enable 8 bit mode
    put32(UART_IMSC, 0);

	put32(UART_CR,(1<<8)|(1<<9)|1);               //Finally, enable transmitter and receiver
}
