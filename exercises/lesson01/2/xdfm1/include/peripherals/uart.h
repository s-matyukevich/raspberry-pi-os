#ifndef	_P_UART_H
#define	_P_UART_H

#include "peripherals/base.h"

#define UART_BASE (PBASE + 0x201000)
#define UART_DR_REG (UART_BASE + 0x00)
#define UART_FR_REG (UART_BASE + 0x18)
#define UART_IBRD_REG (UART_BASE + 0x24)
#define UART_FBRD_REG (UART_BASE + 0x28)
#define UART_LCRH_REG (UART_BASE + 0x2c)
#define UART_CR_REG (UART_BASE + 0x30)
#define UART_IMSC_REG (UART_BASE + 0x38)
#define UART_ICR_REG (UART_BASE + 0x44)

#define UART_LCRH_FEN (1<<4)	// enable FIFO buffer
#define UART_LCRH_WLEN_8BIT (3<<5)	// word length = 8 bits
#define UART_CR_UARTEN 1
#define UART_CR_TXE (1<<8)
#define UART_CR_RXE (1<<9)
#define UARTCLK 48000000

#endif  /*_P_UART_H */
