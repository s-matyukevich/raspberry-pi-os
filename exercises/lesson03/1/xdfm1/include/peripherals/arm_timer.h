#ifndef _P_ARMTIMER_H
#define _P_ARMTIMER_H

#include "peripherals/base.h"

// from section 14.2 of SoC doc

#define ARMTIMER_BASE	(PBASE+0x0000B000)
#define ARMTIMER_LOAD	(ARMTIMER_BASE+0x400)
#define ARMTIMER_VAL    (ARMTIMER_BASE+0x404)
#define ARMTIMER_CTRL  (ARMTIMER_BASE+0x408)
#define ARMTIMER_CLR   (ARMTIMER_BASE+0x40C)

#define ARMTIMER_CTRL_23BIT (1 << 1)
#define ARMTIMER_CTRL_PRESCALE256 (2 << 2)
#define ARMTIMER_CTRL_ENABLE (1 << 7)
#define ARMTIMER_CTRL_INT_ENABLE (1 << 5)

#endif  /*_P_ARMTIMER_H */
