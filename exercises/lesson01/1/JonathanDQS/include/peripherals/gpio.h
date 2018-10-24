#ifndef	_P_GPIO_H
#define	_P_GPIO_H

#include "peripherals/base.h"

//GPIO function select 1
//0x7E200004 QUESTION why base is 0x3F000000 rather than 0X7F?
//Memory management?
//General purpose I/0 function select registers 1
#define GPFSEL1         (PBASE+0x00200004)
//GPIO pin output set 0
#define GPSET0          (PBASE+0x0020001C)
//GPIO pin output clear 0
#define GPCLR0          (PBASE+0x00200028)
//GPIO pin pull up-down enable 0
#define GPPUD           (PBASE+0x00200094)
//GPIO pin pull up-down enable clock 0
#define GPPUDCLK0       (PBASE+0x00200098)

#endif  /*_P_GPIO_H */
