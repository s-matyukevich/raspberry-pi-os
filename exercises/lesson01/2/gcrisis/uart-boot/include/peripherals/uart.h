#ifndef	_P_UART_H
#define	_P_UART_H

#include "peripherals/base.h"

#define UART_DR         (PBASE+0x00201000)
#define UART_RSRECR     (PBASE+0x00201004)
#define UART_FR         (PBASE+0x00201018)
#define UART_IBRD       (PBASE+0x00201024)
#define UART_FBRD       (PBASE+0x00201028)
#define UART_LCRH       (PBASE+0x0020102C)
#define UART_CR         (PBASE+0x00201030)
#define UART_IFLS       (PBASE+0x00201034)
#define UART_IMSC       (PBASE+0x00201038)
#define UART_RIS        (PBASE+0x0020103C)
#define UART_MIS        (PBASE+0x00201040)
#define UART_ICR        (PBASE+0x00201044)
#define UART_DMACR      (PBASE+0x00201048)
#define UART_ITCR       (PBASE+0x00201080)
#define UART_ITIP       (PBASE+0x00201084)
#define UART_ITOP       (PBASE+0x00201088)
#define UART_TDR        (PBASE+0x0020108C)

#endif  /*_P_UART_H */
