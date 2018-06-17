#include "utils.h"
#include "peripherals/uart.h"
#include "peripherals/gpio.h"

void uart_send ( char c )
{	
	// wait if transmit buffer is full (TXFF)
	while(get32(UART_FR_REG) & (1<<5)) {
	}
	put32(UART_DR_REG,c);
}

char uart_recv ( void )
{
	// wait if recv buffer is empty (RXFE)
	while(get32(UART_FR_REG) & (1<<4)) {
	}
	return(get32(UART_DR_REG));
}

void uart_send_string(char* str )
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
	selector |= 4<<15;                      // set alt0 for gpio 15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

	put32(UART_ICR_REG, 0x7ff);	// clear all interrupts
	
	/* divisor = UARTCLK / (16 * baudrate) */
	/* fractional part register = (fractional part of divisor * 64) + 0.5 */
	/* baudrate = 115200 */
	put32(UART_IBRD_REG, 26);	// integer part
	put32(UART_FBRD_REG, 3);	// fractional part
	put32(UART_LCRH_REG, UART_LCRH_FEN | UART_LCRH_WLEN_8BIT); // FIFO buffer, 8bits data 
	put32(UART_IMSC_REG, 0);	// mask all interrupts
	put32(UART_CR_REG, UART_CR_UARTEN | UART_CR_TXE | UART_CR_RXE); // enable uaert, transmt and receive
}
