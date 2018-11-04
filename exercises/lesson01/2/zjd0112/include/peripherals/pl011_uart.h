#ifndef _P_PL011_UART_H
#define _P_PL011_UART_H

#include "peripherals/base.h"

#define UART_FR     (PBASE+0x201018)
#define UART_DR     (PBASE+0x201000)
#define UART_CR     (PBASE+0x201030)
#define UART_LCRH   (PBASE+0x20102C) 
#define UART_IBRD   (PBASE+0x201024)
#define UART_FBRD   (PBASE+0x201028)

#endif  /*_P_PL011_UART_H*/
