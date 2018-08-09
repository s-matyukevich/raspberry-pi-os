#include "utils.h"
#include "printf.h"
#include "peripherals/local_timer.h"

#define RESET_VALUE 38461538

void timer_init ( void )
{
	put32(TIMER_CS,  TIMER_IRQ_EN | TIMER_EN | RESET_VALUE);
	put32(TIMER_LIR, TIMER_IRQ_CORE0);
}

void handle_timer_irq( void ) 
{
	put32(TIMER_ICR, TIMER_IRQ_CLR);
	printf("Timer interrupt received\n\r");
}
