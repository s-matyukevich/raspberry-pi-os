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

#define PACTL_CS_REG (PBASE + 0x00204E00)

#define UART0_BASE (PBASE + 0x00201000)
#define UART2_BASE (PBASE + 0x00201400)
#define UART3_BASE (PBASE + 0x00201600)
#define UART4_BASE (PBASE + 0x00201800)
#define UART5_BASE (PBASE + 0x00201a00)

#define UART_DR_OFFSET 0x0
#define UART_FR_OFFSET 0x18
#define UART_IBRD_OFFSET 0x24
#define UART_FBRD_OFFSET 0x28
#define UART_LCRH_OFFSET 0x2C
#define UART_CR_OFFSET 0x30

// The UART_IFLS Register is the interrupt FIFO level select register.
// You can use this register to define the FIFO level that triggers the
// assertion of the combined interrupt signal.
//
// The interrupts are generated based on a transition through a level rather
// than being based on the level. That is, the interrupts are generated when
//  the fill level progresses through the trigger level. The bits are reset
// so that the trigger level is when the FIFOs are at the half-way mark.
#define UART_IFLS_OFFSET 0x34

// Setting the appropriate mask bit HIGH enables the interrupt.
#define UART_IMSC_OFFSET 0x38

// Masked Interrupt Status Register
// The UARTMIS Register is the masked interrupt status register. It is a read-only register.
// This register returns the current masked status value of the corresponding interrupt. A
// write has no effect.
// All the bits except for the modem status interrupt bits (bits 3 to 0) are cleared to 0 when
// reset. The modem status interrupt bits are undefined after reset. Table 3-16 lists the
// register bit assignments.
#define UART_MIS_OFFSET 0x40

// The UART_ICR Register is the interrupt clear register.
#define UART_ICR_OFFSET 0x44

// The receive interrupt changes state when one of the following events occurs:
// • If the FIFOs are enabled and the receive FIFO reaches the programmed trigger
// level. When this happens, the receive interrupt is asserted HIGH. The receive
// interrupt is cleared by reading data from the receive FIFO until it becomes less
// than the trigger level, or by clearing the interrupt.
// • If the FIFOs are disabled (have a depth of one location) and data is received
// thereby filling the location, the receive interrupt is asserted HIGH. The receive
// interrupt is cleared by performing a single read of the receive FIFO, or by clearing
// the interrupt.
#define UART_IRQ_RXIM_MASK  (1 << 4)

// The receive timeout interrupt is asserted when the receive FIFO is not empty, and no
// more data is received during a 32-bit period. The receive timeout interrupt is cleared
// either when the FIFO becomes empty through reading all the data (or by reading the
// holding register), or when a 1 is written to the corresponding bit of the Interrupt Clear
// Register, UARTICR on page 3-21.
// TODO: 32-bit period - 32 cycles of UART clock?
#define UART_IRQ_RTIM_MASK  (1 << 6)

#define UART_RX_MASK        (1 << 4)

#endif /*_P_MINI_UART_H */
