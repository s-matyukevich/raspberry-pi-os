#include "utils.h"
#include "printf.h"
#include "peripherals/timer.h"
#include "timer.h"

const unsigned int interval = 9600000;
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

void generic_timer_init ( void )
{
	gen_timer_init();
	gen_timer_reset();
}

void handle_generic_timer_irq( void ) 
{
	printf("Timer interrupt received\n\r");
	gen_timer_reset();
}