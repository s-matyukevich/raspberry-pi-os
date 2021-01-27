#ifndef	_P_TIMER_H
#define	_P_TIMER_H

#include "peripherals/base.h"

#define TIMER_CS        (PBASE+0x00003000)
#define TIMER_CLO       (PBASE+0x00003004)
#define TIMER_CHI       (PBASE+0x00003008)
#define TIMER_C0        (PBASE+0x0000300C)
#define TIMER_C1        (PBASE+0x00003010)
#define TIMER_C2        (PBASE+0x00003014)
#define TIMER_C3        (PBASE+0x00003018)

#define TIMER_CS_M0	(1 << 0)
#define TIMER_CS_M1	(1 << 1)
#define TIMER_CS_M2	(1 << 2)
#define TIMER_CS_M3	(1 << 3)

// Local Timer Configuration.
// A free-running secondary timer is provided that can generate an interrupt each time it crosses zero. When it is
// enabled, the timer is decremented on each edge (positive or negative) of the crystal reference clock. It is
// automatically reloaded with the TIMER_TIMEOUT value when it reaches zero and then continues to decrement.
// Routing of the timer interrupt is controlled by the PERI_IRQ_ROUTE0 register
#define LOCAL_TIMER_CONTROL (ARM_LOCAL_BASE + 0x34)

// Local Timer Interrupt Control
#define LOCAL_TIMER_IRQ     (ARM_LOCAL_BASE + 0x38)

// 54 MHz
#define LOCAL_TIMER_FREQ    (54000000)

#endif  /*_P_TIMER_H */