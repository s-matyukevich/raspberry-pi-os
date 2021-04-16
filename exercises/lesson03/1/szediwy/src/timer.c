#include "utils.h"
#include "custom_printf.h"
#include "peripherals/timer.h"

const unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init ( void )
{
	curVal = get32(TIMER_CLO);
	curVal += interval;
	put32(TIMER_C1, curVal);
}

void handle_timer_irq( void ) 
{
	curVal += interval;
	put32(TIMER_C1, curVal);
	put32(TIMER_CS, TIMER_CS_M1);
	printf("Timer interrupt received\n\r");
}

void handle_local_timer_irq( void ) 
{
	printf("LOCAL_TIMER_IRQ received\n\r");
	put32(LOCAL_TIMER_IRQ, (3<<30));	
}

void local_timer_init (void) {	
	// Enable the timer
	put32(LOCAL_TIMER_CONTROL, (3<<28) | (3*LOCAL_TIMER_FREQ));
}