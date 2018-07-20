#include "utils.h"
#include "peripherals/uart_pl011.h"
#include "peripherals/gpio.h"

void uart_send ( char c )
{
	while(1) {
		if(!(get32(UART_FLAG_REG)&0x20)) 
			break;
	}
	put32(UART_DATA_REG,c);
}

char uart_recv ( void )
{
	while(1) {
		if(!(get32(UART_FLAG_REG)&0x10)) 
			break;
	}
	return(get32(UART_DATA_REG)&0xFF);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

void uart_init ( void )
{
	unsigned int selector;

	selector = get32(GPFSEL1);
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 4<<12;                      // set alt5 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 4<<15;                      // set alt5 for gpio15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

	//At startup uart is disabled by default	
	put32(UART_IBRD_REG, UART_IBRD);         //Change baudrate in peripherals/uart_pl011.h
	put32(UART_FBRD_REG, UART_FBRD); 
	put32(UART_LCRH_REG, 0x70);              //Uart hardcoded for for 8 bit mode - no parity - fifo enable
	put32(UART_CTRL_REG, 0x301);             //Enable uart, RX and TX
}
