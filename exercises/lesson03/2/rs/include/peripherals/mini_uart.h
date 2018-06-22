#ifndef _P_MINI_UART_H
#define _P_MINI_UART_H

#include "peripherals/base.h"

#define AUX_ENABLES (PBASE + 0x00215004)
#define AUX_MU_IO_REG (PBASE + 0x00215040)
#define AUX_MU_IER_REG (PBASE + 0x00215044)
#define AUX_MU_IIR_REG (PBASE + 0x00215048)
#define AUX_MU_LCR_REG (PBASE + 0x0021504C)
#define AUX_MU_MCR_REG (PBASE + 0x00215050)
#define AUX_MU_LSR_REG (PBASE + 0x00215054)
#define AUX_MU_MSR_REG (PBASE + 0x00215058)
#define AUX_MU_SCRATCH (PBASE + 0x0021505C)
#define AUX_MU_CNTL_REG (PBASE + 0x00215060)
#define AUX_MU_STAT_REG (PBASE + 0x00215064)
#define AUX_MU_BAUD_REG (PBASE + 0x00215068)

#define MU_IER_RXEN (1 << 0)       // Enable receive interrupts.
#define MU_IER_TXEN (1 << 1)       // Enable transmit interrupts.
#define MU_IER_IGNORED (0x1f << 2) // Ignored but must be set to 1

#define MU_IIR_RX_READY (2 << 1) // Receiver holds valid byte
#define MU_IIR_RX_CLR (1 << 1)   // Clear receiver FIFO

#endif /*_P_MINI_UART_H */
