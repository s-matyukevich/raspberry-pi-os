#include "utils.h"
#include "printf.h"
#include "peripherals/timer.h"

const unsigned int interval = 38461538;

void timer_init ( void )
{
	int enable = 1;
	asm("msr CNTP_CTL_EL0, %0" :: "r" (enable));
	asm("msr CNTP_TVAL_EL0, %0" :: "r" (interval));
}

void handle_timer_irq( void ) 
{
	asm("msr CNTP_TVAL_EL0, %0" :: "r" (interval));
	printf("Timer interrupt received\n\r");
}
