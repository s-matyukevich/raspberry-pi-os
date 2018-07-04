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

// See BCM2836 ARM-local peripherals at
// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2836/QA7_rev3.4.pdf

#define LTIMER_CTRL    (LPBASE+0x34)
#define LTIMER_CLR     (LPBASE+0x38)

#define LTIMER_CTRL_EN     (1 << 28)
#define LTIMER_CTRL_INT_EN (1 << 29)
#define LTIMER_CTRL_VALUE  (LTIMER_CTRL_EN | LTIMER_CTRL_INT_EN)

#define LTIMER_CLR_ACK (1 << 31)

#endif  /*_P_TIMER_H */
