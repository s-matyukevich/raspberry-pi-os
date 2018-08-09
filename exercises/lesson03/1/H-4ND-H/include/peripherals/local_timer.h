#ifndef	_P_TIMER_H
#define	_P_TIMER_H

#include "peripherals/base.h"

#define TIMER_CS        (0x40000034)
#define TIMER_ICR       (0x40000038)
#define TIMER_LIR       (0x40000024)


#define TIMER_IRQ_CLR	(1 << 31)
#define TIMER_IRQ_CORE0	(0)
#define TIMER_IRQ_EN	(1 << 29)
#define TIMER_EN	(1 << 28)

#endif  /*_P_TIMER_H */
