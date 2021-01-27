#ifndef	_P_GPIO_H
#define	_P_GPIO_H

#include "peripherals/base.h"

#define GPFSEL1         (PBASE+0x00200004)
// #define GPSET0          (PBASE+0x0020001C)
// #define GPCLR0          (PBASE+0x00200028)
// #define GPPUD           (PBASE+0x00200094)
// #define GPPUDCLK0       (PBASE+0x00200098)
#define GPIO_PUP_PDN_CNTRL_REG0 (PBASE+0x002000E4)
#define UART0_DR                (PBASE+0x00201000)
#define UART0_FR                (PBASE+0x00201018)
#define UART0_IBRD              (PBASE+0x00201024)
#define UART0_FBRD              (PBASE+0x00201028)
#define UART0_LCRH              (PBASE+0x0020102C)
#define UART0_CR                (PBASE+0x00201030)
#define UART0_IMSC              (PBASE+0x00201038)

#endif  /*_P_GPIO_H */
