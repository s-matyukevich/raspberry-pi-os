#ifndef _SYSREGS_H
#define _SYSREGS_H

#define SCR_SECURE_64BIT		(1 << 10)

#define SPSR_MASK_ALL 			(7 << 6)
#define SPSR_EL1h				(5 << 0)
#define SPSR_VALUE				(SPSR_MASK_ALL | SPSR_EL1h)

#define ESR_ELx_EC_SHIFT		26
#define ESR_ELx_EC_SVC64		0x15

#endif
