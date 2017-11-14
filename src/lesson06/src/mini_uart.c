#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "utils.h"
#include "peripherals/mini_uart.h"
#include "peripherals/gpio.h"

void uart_send ( char c )
{
	while(1)
	{
		if(get32(AUX_MU_LSR_REG)&0x20) break;
	}
	put32(AUX_MU_IO_REG,c);
}

char uart_recv ( void )
{
	while(1)
	{
		if(get32(AUX_MU_LSR_REG)&0x01) break;
	}
	return(get32(AUX_MU_IO_REG)&0xFF);
}



void uart_init ( void )
{
	uint32_t ra;

	ra=get32(GPFSEL1);
	ra&=~(7<<12); //gpio14
	ra|=2<<12;    //alt5
	ra&=~(7<<15); //gpio15
	ra|=2<<15;    //alt5
	put32(GPFSEL1,ra);
	put32(GPPUD,0);
	delay(150);
	put32(GPPUDCLK0,(1<<14)|(1<<15));
	delay(150);
	put32(GPPUDCLK0,0);

	put32(AUX_ENABLES,1); 		//Enable mini uart (this also enables access to it registers)
	put32(AUX_MU_CNTL_REG,0);	//Disable auto flow control and disable receiver and transmitter (for now)
	put32(AUX_MU_IER_REG,0); 	//Disable receive and transmit interrupts
	put32(AUX_MU_LCR_REG,3);	//Enable 8 bit mode
	put32(AUX_MU_MCR_REG,0);	//Set RTS line to be always high
	put32(AUX_MU_BAUD_REG,270);	//Set baund rate to 115200
	put32(AUX_MU_IIR_REG,6);	//Clear FIFO

	put32(AUX_MU_CNTL_REG,3);	//Finaly, enable transmitter and receiver
}


// This function is required by printf function
void putc ( void* p, char c)
{
	uart_send(c);
}
