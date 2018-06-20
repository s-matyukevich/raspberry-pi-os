#ifndef _SYSREGS_H
#define _SYSREGS_H

// ***************************************
// SCTLR_EL1, System Control Register (EL1), Page 2025 of AArch64-Reference-Manual.
// ***************************************

#define SCTLR_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define SCTLR_EE_LITTLE_ENDIAN          (0 << 25)
#define SCTLR_EOE_LITTLE_ENDIAN         (0 << 24)
#define SCTLR_I_CACHE_DISABLED          (0 << 12)
#define SCTLR_D_CACHE_DISABLED          (0 << 2)
#define SCTLR_MMU_DISABLED              (0 << 0)
#define SCTLR_MMU_ENABLED               (1 << 0)

#define SCTLR_VALUE_MMU_DISABLED	(SCTLR_RESERVED | SCTLR_EE_LITTLE_ENDIAN | SCTLR_I_CACHE_DISABLED | SCTLR_D_CACHE_DISABLED | SCTLR_MMU_DISABLED)

// ***************************************
// HCR_EL2, Hypervisor Configuration Register (EL2), Page 1923 of AArch64-Reference-Manual.
// ***************************************

#define HCR_RW	    			(1 << 31)
#define HCR_VALUE			HCR_RW

// ***************************************
// SCR_EL3, Secure Configuration Register (EL3), Page 2022 of AArch64-Reference-Manual.
// ***************************************

#define SCR_RESERVED	    		(3 << 4)
#define SCR_RW				(1 << 10)
#define SCR_NS				(1 << 0)
#define SCR_VALUE	    	    	(SCR_RESERVED | SCR_RW | SCR_NS)

// ***************************************
// SPSR_EL3, Saved Program Status Register (EL3) Page 288 of AArch64-Reference-Manual.
// ***************************************

#define SPSR_MASK_ALL 			(7 << 6)
#define SPSR_EL1h			(0b0101)
#define SPSR_EL2h           (0b1001)
#define SPSR_EL1			(SPSR_MASK_ALL | SPSR_EL1h)
#define SPSR_EL2			(SPSR_MASK_ALL | SPSR_EL2h)

#endif
