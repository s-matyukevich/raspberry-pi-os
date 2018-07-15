#ifndef P_UART_H
#define P_UART_H
#include "peripherals/base.h"

#define UART_OFFS 0x00201000

#define UART_DR     (PBASE + UART_OFFS + 0x00)
#define UART_RSRECR (PBASE + UART_OFFS + 0x04)
#define UART_FR     (PBASE + UART_OFFS + 0x18)
#define UART_ILPR   (PBASE + UART_OFFS + 0x20)
#define UART_IBRD   (PBASE + UART_OFFS + 0x24)
#define UART_FBRD   (PBASE + UART_OFFS + 0x28)
#define UART_LCRH   (PBASE + UART_OFFS + 0x2C)
#define UART_CR     (PBASE + UART_OFFS + 0x30)
#define UART_IFLS   (PBASE + UART_OFFS + 0x34)
#define UART_IMSC   (PBASE + UART_OFFS + 0x38)
#define UART_RIS    (PBASE + UART_OFFS + 0x3C)
#define UART_MIS    (PBASE + UART_OFFS + 0x40)
#define UART_ICR    (PBASE + UART_OFFS + 0x44)
#define UART_DMACR  (PBASE + UART_OFFS + 0x48)
#define UART_ITCR   (PBASE + UART_OFFS + 0x80)
#define UART_ITIP   (PBASE + UART_OFFS + 0x84)
#define UART_ITOP   (PBASE + UART_OFFS + 0x88)
#define UART_TDR    (PBASE + UART_OFFS + 0x8C)

#endif /* P_UART_H */
