#ifndef _P_UART_H
#define _P_UART_H

#include "peripherals/base.h"

#define UART_BASE (PBASE + 0x201000)
#define UART_DR_REG (UART_BASE + 0x0)    // Data Register
#define UART_FR_REG (UART_BASE + 0x18)   // Flag register
#define UART_IBRD_REG (UART_BASE + 0x24) // Integer Baud rate divisor
#define UART_FBRD_REG (UART_BASE + 0x28) // Fractional Baud rate divisor
#define UART_LCRH_REG (UART_BASE + 0x2c) // Line Control register
#define UART_CR_REG (UART_BASE + 0x30)   // Control register

// UART_CR_REG fields
#define CR_ENABLE 1     // UART enable
#define CR_TXE (1 << 8) // Transmit enable
#define CR_RXE (1 << 9) // Receive enable

// UART_LCRH_REG fields
#define LCRH_FEN (1 << 4) // Enable FIFOs
#define LCRH_WLEN_8BITS (3 << 5)

#define IBRD_BDIVINT 0xffff // Significant part of int. divisor value
#define FBRD_BDIVFRAC 0x1f  // Significant part of frac. divisor value

#define UART_CLOCK 48000000 // Assuming 48Mhz (TODO: use mailbox to get this value)
#define UART_BAUD_RATE 115200

#endif /*_P_UART_H */
