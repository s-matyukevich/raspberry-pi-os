#ifndef	_P_BASE_H
#define	_P_BASE_H

// #define PBASE 0x3F000000
// So a peripheral described in this document as being at legacy address 0x7Enn_nnnn is available in the 35-bit address
// space at 0x4_7Enn_nnnn, and visible to the ARM at 0x0_FEnn_nnnn if Low Peripheral mode is enabled.
// 0x7E000000 (legacy) -> 0x4_7E00_0000 (35-bit) -> 0x0_FE00_0000 (low peripheral)
#define PBASE 0xFE000000

// The base address of the GIC-400 is 0x4c0040000. Note that, unlike other peripheral addresses in this document, this is an
// ARM-only address and not a legacy master address. If Low Peripheral mode is enabled this base address becomes
// 0xff840000.
// The GIC-400 is configured with "NUM_CPUS=4" and "NUM_SPIS=192". For full register details, please refer to the ARM
// GIC-400 documentation on the ARM Developer website.
#define GIC_BASE 0xFF840000

#define ARM_LOCAL_BASE 0xFF800000

// The ARMC register base address is 0x7e00b000 -> 0x4_7E00B000 -> 0x0FE00B000
#define ARMC_BASE 0x0FE00B000

#endif  /*_P_BASE_H */
