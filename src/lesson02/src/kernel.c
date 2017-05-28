#include <stddef.h>
#include <stdint.h>

#include "printf.h"
#include "boot.h"

#define PBASE 0x3F000000
#define GPFSEL1         (PBASE+0x00200004)
#define GPSET0          (PBASE+0x0020001C)
#define GPCLR0          (PBASE+0x00200028)
#define GPPUD           (PBASE+0x00200094)
#define GPPUDCLK0       (PBASE+0x00200098)

#define AUX_ENABLES     (PBASE+0x00215004)
#define AUX_MU_IO_REG   (PBASE+0x00215040)
#define AUX_MU_IER_REG  (PBASE+0x00215044)
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
#define AUX_MU_MCR_REG  (PBASE+0x00215050)
#define AUX_MU_LSR_REG  (PBASE+0x00215054)
#define AUX_MU_MSR_REG  (PBASE+0x00215058)
#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
#define AUX_MU_CNTL_REG (PBASE+0x00215060)
#define AUX_MU_STAT_REG (PBASE+0x00215064)
#define AUX_MU_BAUD_REG (PBASE+0x00215068)

char uart_recv ( void )
{
	while(1)
	{
		if(GET32(AUX_MU_LSR_REG)&0x01) break;
	}
	return(GET32(AUX_MU_IO_REG)&0xFF);
}

void uart_send ( char c )
{
	while(1)
	{
		if(GET32(AUX_MU_LSR_REG)&0x20) break;
	}
	PUT32(AUX_MU_IO_REG,c);
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

void putc ( void* p, char c)
{
	uart_send(c);
}

void uart_send_string(char* str)
{
       for (size_t i = 0; str[i] != '\0'; i ++)
       {
               uart_send((char)str[i]);
       }
}

void kernel_main(void)
{
	uart_init();
	int el = GET_EL();
	printf("Exception level: %d \r\n", el);

	while (1)
		uart_send(uart_recv()); 
}
