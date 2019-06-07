#include "uart.h"
#include <stdint.h>
extern void BRANCHTO(unsigned int);

void check_head(char *h,int hlen)
{
    uint8_t index=0;
    while(1)
	{
		char c=uart_recv();
		if(c==h[index])
		{
			index++;
		}
		else
		{
			if(c==h[0])
				index=1;
			else
				index=0;
		}
		if(index==hlen)
			break;
	}
	uart_send_string("OK");
}
void kernel_main(void)
{   
	uart_init(115200);
	while (1) {
	    //check_begin 
		
		char *starth="start";
		check_head(starth,5);
		uint32_t size = uart_recv();
		size|=uart_recv()<<8;
		size|=uart_recv()<<16;
		size|=uart_recv()<<24;
		
		uart_send((size>>24)&0xff);
		uart_send((size>>16)&0xff);
		uart_send((size>>8)&0xff);
		uart_send(size&0xff);
				
		uint8_t * kernel = (uint8_t *)0x0;
		while(size-- >0)
		{
			*kernel++ = uart_recv();
		}		    
    	BRANCHTO(0);    
	}
	return ;
}
