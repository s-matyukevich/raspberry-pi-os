#include "utils.h"
#include "printf.h"
#include "peripherals/timer.h"

const unsigned int interval = 20000000;
unsigned int curVal = 0;

void timer_init ( void )
{
	// Set value, enable Timer and Interrupt
	put32(TIMER_CTRL, ((1<<28) | interval));
}

void handle_timer_irq( void ) 
{
	printf("Timer interrupt received, Local Timer\n\r");
	put32(TIMER_FLAG, (3<<30));				// clear interrupt flag and reload timer

}
