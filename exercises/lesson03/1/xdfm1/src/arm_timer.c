#include "utils.h"
#include "printf.h"
#include "peripherals/arm_timer.h"

// counter value in load register
const unsigned int interval = 0x800;

void armtimer_init ( void )
{
	put32(ARMTIMER_LOAD, interval);
	put32(ARMTIMER_CTRL, ARMTIMER_CTRL_23BIT | 
			    ARMTIMER_CTRL_PRESCALE256 |		//presclaer clk/256
			    ARMTIMER_CTRL_ENABLE |
			    ARMTIMER_CTRL_INT_ENABLE); 
}

void handle_armtimer_irq( void ) 
{
	put32(ARMTIMER_CLR, 1);
	printf("Timer iterrupt received\n\r");
}
