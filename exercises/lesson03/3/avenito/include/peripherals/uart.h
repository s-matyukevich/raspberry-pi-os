#ifndef	_P_UART_H
#define	_P_UART_H

#include "peripherals/base.h"

#define AUX_ENABLES     (PBASE+0x00201000)

#define UART_DR			(PBASE+0x00201000)
#define UART_FR			(PBASE+0x00201018)
#define UART_CR			(PBASE+0x00201030)
#define UART_IBRD		(PBASE+0x00201024)
#define UART_FBRD		(PBASE+0x00201028)
#define UARTLCR_LCRH	(PBASE+0x0020102C)




#endif  /*_P_UART_H */
