#ifndef	_P_BASE_H
#define	_P_BASE_H

// #define PBASE 0x3F000000
// So a peripheral described in this document as being at legacy address 0x7Enn_nnnn is available in the 35-bit address
// space at 0x4_7Enn_nnnn, and visible to the ARM at 0x0_FEnn_nnnn if Low Peripheral mode is enabled.
// 0x7E000000 (legacy) -> 0x4_7E00_0000 (35-bit) -> 0x0_FE00_0000 (low peripheral)
#define PBASE 0xFE000000

#endif  /*_P_BASE_H */
