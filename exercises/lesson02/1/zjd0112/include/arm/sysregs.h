#ifndef _SYSREGS_H
#define _SYSREGS_H

// ***************************************
// SCTLR_EL1, System Control Register (EL1), Page 2654 of AArch64-Reference-Manual.
// ***************************************

#define SCTLR1_RESERVED                  (3 << 28) | (3 << 22) | (1 << 20) | (1 << 11)
#define SCTLR1_EE_LITTLE_ENDIAN          (0 << 25)
#define SCTLR1_EOE_LITTLE_ENDIAN         (0 << 24)
#define SCTLR1_I_CACHE_DISABLED          (0 << 12)
#define SCTLR1_D_CACHE_DISABLED          (0 << 2)
#define SCTLR1_MMU_DISABLED              (0 << 0)
#define SCTLR1_MMU_ENABLED               (1 << 0)

#define SCTLR1_VALUE_MMU_DISABLED	(SCTLR1_RESERVED | SCTLR1_EE_LITTLE_ENDIAN | SCTLR1_I_CACHE_DISABLED | SCTLR1_D_CACHE_DISABLED | SCTLR1_MMU_DISABLED)

// ***************************************
// SCTLR_EL2, System Control Register (EL2), Page 2665 of AArch64-Reference-Manual.
// ***************************************
#define SCTLR2_RESERVED                 (3 << 28) | (3 << 22) | (1 << 18) | (1 << 16) | (1 << 11) | (1 << 4)
#define SCTLR2_EE_LITTLE_ENDIAN         (0 << 25)
#define SCTLR2_I_CACHE_DISABLED         (0 << 12)
#define SCTLR2_D_CACHE_DISABLED         (0 << 2)
#define SCTLR2_MMU_DISABLED             (0 << 0)
#define SCTLR2_MMU_ENABLED              (1 << 0)

#define SCTLR2_VALUE_MMU_DISABLED   (SCTLR2_RESERVED | SCTLR2_EE_LITTLE_ENDIAN | SCTLR2_I_CACHE_DISABLED | SCTLR2_D_CACHE_DISABLED | SCTLR2_MMU_DISABLED)
// ***************************************
// HCR_EL2, Hypervisor Configuration Register (EL2), Page 2487 of AArch64-Reference-Manual.
// ***************************************

#define HCR_RW	    			(1 << 31)
#define HCR_VALUE			HCR_RW

// ***************************************
// SCR_EL3, Secure Configuration Register (EL3), Page 2648 of AArch64-Reference-Manual.
// ***************************************

#define SCR_RESERVED	    		(3 << 4)
#define SCR_RW				(1 << 10)
#define SCR_NS				(1 << 0)
#define SCR_VALUE	    	    	(SCR_RESERVED | SCR_RW | SCR_NS)

// ***************************************
// SPSR_EL3, Saved Program Status Register (EL3) Page 389 of AArch64-Reference-Manual.
// ***************************************

#define SPSR_MASK_ALL_EL3 		(7 << 6)
#define SPSR_EL2h			    (9 << 0)
#define SPSR_VALUE_EL3			(SPSR_MASK_ALL_EL3 | SPSR_EL2h)

// ***************************************
// SPSR_EL2, Saved Program Status Register (EL2) Page 383 of AArch64-Reference-Manual.
// ***************************************

#define SPSR_MASK_ALL_EL2       (7 << 6)
#define SPSR_EL1h               (5 << 0)
#define SPSR_VALUE_EL2          (SPSR_MASK_ALL_EL2 | SPSR_EL1h)

#endif
