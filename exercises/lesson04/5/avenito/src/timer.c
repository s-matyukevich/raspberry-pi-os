#include "utils.h"
#include "printf.h"
#include "sched.h"
#include "timer.h"
#include "peripherals/timer.h"

const unsigned int interval = 200000;
unsigned int curVal = 0;

void timer_init ( void )
{
	// Set value, enable Timer and Interrupt
	put32(TIMER_CTRL, ((1<<28) | interval));
}

void handle_timer_irq( void ) 
{
	printf("Timer interrupt received, Generic Timer\n\r");
	generic_timer_reset();
	timer_tick();
}
