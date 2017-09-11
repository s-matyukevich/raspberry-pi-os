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
		if(GET32(AUX_MU_LSR_REG)&0x20) break;
	}
	PUT32(AUX_MU_IO_REG,c);
}

char uart_recv ( void )
{
	while(1)
	{
		if(GET32(AUX_MU_LSR_REG)&0x01) break;
	}
	return(GET32(AUX_MU_IO_REG)&0xFF);
}



void uart_init ( void )
{
	uint32_t ra;

	ra=GET32(GPFSEL1);
	ra&=~(7<<12); //gpio14
	ra|=2<<12;    //alt5
	ra&=~(7<<15); //gpio15
	ra|=2<<15;    //alt5
	PUT32(GPFSEL1,ra);
	PUT32(GPPUD,0);
	DELAY(150);
	PUT32(GPPUDCLK0,(1<<14)|(1<<15));
	DELAY(150);
	PUT32(GPPUDCLK0,0);

	PUT32(AUX_ENABLES,1); 		//Enable mini uart (this also enables access to it registers)
	PUT32(AUX_MU_CNTL_REG,0);	//Disable auto flow control and disable receiver and transmitter (for now)
	PUT32(AUX_MU_IER_REG,0); 	//Disable receive and transmit interrupts
	PUT32(AUX_MU_LCR_REG,3);	//Enable 8 bit mode
	PUT32(AUX_MU_MCR_REG,0);	//Set RTS line to be always high
	PUT32(AUX_MU_BAUD_REG,270);	//Set baund rate to 115200
	PUT32(AUX_MU_IIR_REG,6);	//Clear FIFO

	PUT32(AUX_MU_CNTL_REG,3);	//Finaly, enable transmitter and receiver
}


// This function is required by printf function
void putc ( void* p, char c)
{
	uart_send(c);
}
