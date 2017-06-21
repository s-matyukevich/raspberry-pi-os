#ifndef	_P_TIMER_H
#define	_P_TIMER_H

#include "peripherals/base.h"

#define TIMER_CTRL_23BIT         ( 1 << 1 ) // 0 : 16-bit counters - 1 : 23-bit counter 

#define TIMER_CTRL_PRESCALE_1    ( 0 << 2 )
#define TIMER_CTRL_PRESCALE_16   ( 1 << 2 )
#define TIMER_CTRL_PRESCALE_256  ( 2 << 2 )

#define TIMER_CTRL_INT_ENABLE    ( 1 << 5 ) //0 : Timer interrupt disabled - 1 : Timer interrupt enabled
#define TIMER_CTRL_INT_DISABLE   ( 0 << 5 )

#define TIMER_CTRL_ENABLE        ( 1 << 7 ) //0 : Timer disabled - 1 : Timer enabled
#define TIMER_CTRL_DISABLE       ( 0 << 7 )


#define TIMER_LOAD						(PBASE+0x0000B400)
#define TIMER_VALUE						(PBASE+0x0000B404)
#define TIMER_CONTROL					(PBASE+0x0000B408)
#define TIMER_IRQ_CLEAR_ACK				(PBASE+0x0000B40C)
#define TIMER_RAW_IRQ					(PBASE+0x0000B410)
#define TIMER_MASKED_IRQ				(PBASE+0x0000B414)
#define TIMER_RELOAD					(PBASE+0x0000B418)
#define TIMER_PRE_DEVIDER				(PBASE+0x0000B41C)
#define TIMER_PRE_FREE_RUNNING_COUNTER	(PBASE+0x0000B420)

#endif  /*_P_TIMER_H */
