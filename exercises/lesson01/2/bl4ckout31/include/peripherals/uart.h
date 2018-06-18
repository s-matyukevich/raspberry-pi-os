#ifndef	_P_UART_H
#define	_P_UART_H

#include "peripherals/base.h"

#define U_BASE       (PBASE+0x00201000)

#define U_DATA_REG   (U_BASE)
#define U_FR_REG     (U_BASE+0x18)
#define U_IBRD_REG   (U_BASE+0x24)
#define U_FBRD_REG   (U_BASE+0x28)
#define U_LCRH_REG   (U_BASE+0x2C)
#define U_CR_REG     (U_BASE+0x30)
#define U_IMSC_REG   (U_BASE+0x38)

#endif  /*_P_UART_H */
