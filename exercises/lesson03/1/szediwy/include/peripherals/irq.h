#ifndef	_P_IRQ_H
#define	_P_IRQ_H

#include "peripherals/base.h"



#define GIC_DIST_BASE		        (GIC_BASE+0x00001000)
#define GIC_CPU_BASE		        (GIC_BASE+0x00002000)



// For interrupt ID m, when DIV and MOD are the integer division and modulo operations:
// • the corresponding GICD_ISENABLER number, n, is given by n = m DIV 32
// • the offset of the required GICD_ISENABLER is (0x100 + (4*n))
// • the bit number of the required Set-enable bit in this register is m MOD 32.
//
// VC Timer 1 = 1 => 97 / 32 = 3 => offset 0x10C
// 
#define GIC_ENABLE_IRQ_BASE         (GIC_DIST_BASE+0x00000100)
#define GIC_ENABLE_IRQ_3            (GIC_DIST_BASE+0x0000010C)

#define GICC_IAR                    (GIC_CPU_BASE+0x0000000C)
#define GICC_EOIR                   (GIC_CPU_BASE+0x00000010)

// For interrupt ID m, when DIV and MOD are the integer division and modulo operations:
// • the corresponding GICD_ITARGETSRn number, n, is given by n = m DIV 4
// • the offset of the required GICD_ITARGETSR is (0x800 + (4*n))
// • the byte offset of the required Priority field in this register is m MOD 4, where:
// — byte offset 0 refers to register bits [7:0]
// — byte offset 1 refers to register bits [15:8]
// — byte offset 2 refers to register bits [23:16]
// — byte offset 3 refers to register bits [31:24].
#define GIC_IRQ_TARGET_BASE            (GIC_DIST_BASE+0x00000800)

// 97 / 4 => n = 24
// 0x800 + 4*24
// 97 % 4 => m = 1
// byte offset = 1<<8
#define GIC_IRQ_TARGET_24           (GIC_DIST_BASE+0x00000860)

#define LOCAL_TIMER_IRQ	    (0x35) // Local timer IRQ
#define SYSTEM_TIMER_IRQ_0	(0x60) // VC IRQ 0
#define SYSTEM_TIMER_IRQ_1	(0x61) // VC IRQ 1
#define SYSTEM_TIMER_IRQ_2	(0x62) // VC IRQ 2 
#define SYSTEM_TIMER_IRQ_3	(0x63) // VC IRQ 3

#endif  /*_P_IRQ_H */