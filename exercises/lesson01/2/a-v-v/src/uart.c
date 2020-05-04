#include "utils.h"
#include "peripherals/uart.h"
#include "peripherals/gpio.h"

void uart_send ( char c )
{
	while(get32(UART_FR) & (1u<<5)) // wait while TXFF is set
		;
	put32(UART_DR,c);
}

char uart_recv ( void )
{
	while(get32(UART_FR) & (1u<<4)) // wait while RXFE is set
		;
	return (get32(UART_DR)&0xFF);
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
	selector |= 4<<12;                      // set alt0 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 4<<15;                      // set alt0 for gpio15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

	const unsigned int BAUDS = 115200;
	const unsigned int IBRD = 0xFFFF & (UART_CLK_HZ / (16 * BAUDS));
	const unsigned int FBRD = 0x3f & (UART_CLK_HZ % (16 * BAUDS) * 64 / UART_CLK_HZ);
	put32(UART_IBRD, IBRD);
	put32(UART_FBRD, FBRD);
	put32(UART_LCRH, (3u << 5));                // set WLEN to 0b11
	put32(UART_CR, (1u << 9) | (1u << 8) | 1u); // set RXE, TXE, UARTEN bits
}
