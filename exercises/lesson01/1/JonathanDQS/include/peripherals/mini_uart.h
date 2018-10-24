#ifndef	_P_MINI_UART_H
#define	_P_MINI_UART_H

#include "peripherals/base.h"

//Base is the start of memory reserved for devices by Pi 3
#define AUX_ENABLES     (PBASE+0x00215004)
//Mini UART I/O data
#define AUX_MU_IO_REG   (PBASE+0x00215040)
//Mini UART Interrupt Enable
#define AUX_MU_IER_REG  (PBASE+0x00215044)
//Mini UART Interrupt Identify
#define AUX_MU_IIR_REG  (PBASE+0x00215048)
//Mini UART Line Control
#define AUX_MU_LCR_REG  (PBASE+0x0021504C)
//Mini UART Modem Control
#define AUX_MU_MCR_REG  (PBASE+0x00215050)
//Mini UART Line Status
#define AUX_MU_LSR_REG  (PBASE+0x00215054)
//Mini UART Modem Status
#define AUX_MU_MSR_REG  (PBASE+0x00215058)
//Mini UART Scratch
#define AUX_MU_SCRATCH  (PBASE+0x0021505C)
//Mini UART Extra Control
#define AUX_MU_CNTL_REG (PBASE+0x00215060)
//Mini UART Extra Status
#define AUX_MU_STAT_REG (PBASE+0x00215064)
//Mini UART Baudrate
#define AUX_MU_BAUD_REG (PBASE+0x00215068)

#endif  /*_P_MINI_UART_H */
