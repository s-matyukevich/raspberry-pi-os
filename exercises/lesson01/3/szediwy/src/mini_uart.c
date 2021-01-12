#include "utils.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

void uart_send(char c)
{
	while (get32(UART0_FR) & (1 << 5))
	{
	}
	put32(UART0_DR, c);
}

char uart_recv(void)
{
	while (get32(UART0_FR) & (1 << 4))
	{
	}
	return (get32(UART0_DR) & 0xFF);
}

void uart_send_string(char *str)
{
	for (int i = 0; str[i] != '\0'; i++)
	{
		uart_send((char)str[i]);
	}
}

void uart_init(void)
{	

	unsigned int selector;

	selector = get32(GPFSEL1);
	selector &= ~(7 << 12); // clean gpio14
	selector |= 4 << 12;	// set alt5 for gpio14
	selector &= ~(7 << 15); // clean gpio15
	selector |= 4 << 15;	// set alt5 for gpio15
	put32(GPFSEL1, selector);

	unsigned int pullRegister;
	pullRegister = get32(GPIO_PUP_PDN_CNTRL_REG0);
	pullRegister &= ~(3 << 30);
	pullRegister &= ~(3 << 28);
	put32(GPIO_PUP_PDN_CNTRL_REG0, pullRegister);

	//first disable uart
	put32(UART0_CR, 0);
	put32(UART0_IMSC, 0);

	//from ../adkaster/src/uart.c
	// Assume 48MHz UART Reference Clock (Standard)
	// Calculate UART clock divider per datasheet
	// BAUDDIV = (FUARTCLK/(16 Baud rate))
	//  Note: We get 6 bits of fraction for the baud div
	// 48000000/(16 * 115200) = 3000000/115200 = 26.0416666...
	// Integer part = 26 :)
	// From http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0183g/I54603.html
	// we want floor(0.04166666.. * 64 + 0.5) = 3
	put32(UART0_IBRD, 26);
	put32(UART0_FBRD, 3);

	//little endian: 0111|0000 => 8 bits and enable fifos
	put32(UART0_LCRH, 7 << 4);
	//little endian: 0011|0000|0001 => enable: rx tx uart
	put32(UART0_CR, (1 << 9) | (1 << 8) | (1 << 0));
}
