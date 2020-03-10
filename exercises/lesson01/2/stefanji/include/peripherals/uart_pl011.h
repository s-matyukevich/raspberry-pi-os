#ifndef _P_UART_PL011_H
#define _P_UART_PL011_H

#include "base.h"

#define PL011_BASE    (PBASE+0x00201000)
#define PL011_DR      (PL011_BASE+0x00)
#define PL011_RSRECR  (PL011_BASE+0x04)
#define PL011_FR      (PL011_BASE+0x18)
#define PL011_ILPR    (PL011_BASE+0x20)
#define PL011_IBRD    (PL011_BASE+0x24)
#define PL011_FBRD    (PL011_BASE+0x28)
#define PL011_LCRH    (PL011_BASE+0x2c)
#define PL011_CR      (PL011_BASE+0x30)
#define PL011_IFLS    (PL011_BASE+0x34)
#define PL011_IMSC    (PL011_BASE+0x38)
#define PL011_RIS     (PL011_BASE+0x3c)
#define PL011_MIS     (PL011_BASE+0x40)
#define PL011_ICR     (PL011_BASE+0x44)
#define PL011_DMACR   (PL011_BASE+0x48)
#define PL011_ITCR    (PL011_BASE+0x80)
#define PL011_ITIP    (PL011_BASE+0x84)
#define PL011_ITOP    (PL011_BASE+0x88)
#define PL011_TDR     (PL011_BASE+0x8c)

#endif

