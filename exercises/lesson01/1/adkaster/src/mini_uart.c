#include "utils.h"
#include "mini_uart.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"
#include <stdint.h>

void uart_send ( char c )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x20) 
			break;
	}
	put32(AUX_MU_IO_REG,c);
}

char uart_recv ( void )
{
	while(1) {
		if(get32(AUX_MU_LSR_REG)&0x01) 
			break;
	}
	return(get32(AUX_MU_IO_REG)&0xFF);
}

void uart_send_string(char* str)
{
	for (int i = 0; str[i] != '\0'; i ++) {
		uart_send((char)str[i]);
	}
}

void uart_init ( miniuart_baud_t baud_rate )
{
	unsigned int selector;

	/* Final baudrate = system_clock_freq / (8 * ( baudrate_reg + 1 ))
	 *  From docs, system_clock_freq = 250MHz 
	 *  Solve for baudrate_reg:
	 *       baudrate_reg = (system_clock_freq / (8*baudrate)) - 1 
	 *       baudrate_reg = 31.25MHz/baudrate - 1
	 *       baudrate_reg = 31250000/baudrate -1
	 * baudrate_reg is a 32 bit register, but per BCM2835-ARM-Peripherals.pdf, only bottom 16 are read
	 */
	uint32_t baudrate_reg = 31250000/baud_rate - 1;
	baudrate_reg &= 0x0000FFFF;

	selector = get32(GPFSEL1);
	selector &= ~(7<<12);                   // clean gpio14
	selector |= 2<<12;                      // set alt5 for gpio14
	selector &= ~(7<<15);                   // clean gpio15
	selector |= 2<<15;                      // set alt5 for gpio15
	put32(GPFSEL1,selector);

	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

	put32(AUX_ENABLES,1);                   //Enable mini uart (this also enables access to it registers)
	put32(AUX_MU_CNTL_REG,0);               //Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0);                //Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG,3);                //Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);                //Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,baudrate_reg);    //Set baud rate based on input

	put32(AUX_MU_CNTL_REG,3);               //Finally, enable transmitter and receiver
}
