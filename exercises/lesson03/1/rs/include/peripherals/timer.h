#ifndef _P_TIMER_H
#define _P_TIMER_H

#include "peripherals/base.h"

#define TIMER_CS (PBASE + 0x00003000)
#define TIMER_CLO (PBASE + 0x00003004)
#define TIMER_CHI (PBASE + 0x00003008)
#define TIMER_C0 (PBASE + 0x0000300C)
#define TIMER_C1 (PBASE + 0x00003010)
#define TIMER_C2 (PBASE + 0x00003014)
#define TIMER_C3 (PBASE + 0x00003018)

#define TIMER_CS_M0 (1 << 0)
#define TIMER_CS_M1 (1 << 1)
#define TIMER_CS_M2 (1 << 2)
#define TIMER_CS_M3 (1 << 3)

// Reference: 14.2 Timer Registers
// https://web.stanford.edu/class/cs140e/docs/BCM2837-ARM-Peripherals.pdf

#define ARM_TIMER_BASE (PBASE + 0xB000)
#define ARM_TIMER_LOAD (ARM_TIMER_BASE + 0x400)
#define ARM_TIMER_VALUE (ARM_TIMER_BASE + 0x404)
#define ARM_TIMER_CTRL (ARM_TIMER_BASE + 0x408)
#define ARM_TIMER_CLR (ARM_TIMER_BASE + 0x40c)

#define CTRL_23BIT (1 << 1)      // 23-bit counter
#define CTRL_INT_ENABLE (1 << 5) // Timer interrupt enabled
#define CTRL_ENABLE (1 << 7)     // Timer enabled

#endif /*_P_TIMER_H */
