#ifndef	_P_IRQ_H
#define	_P_IRQ_H

#include "peripherals/base.h"

#define ARM_TIMER_IRQ         (1 << 0)
#define ARM_MAILBOX_IRQ       (1 << 1)
#define ARM_DOORBELL_0_IRQ    (1 << 2)
#define ARM_DOORBELL_1_IRQ    (1 << 3)
#define GPU_0_HALTED_IRQ      (1 << 4)
#define GPU_1_HALTED_IRQ      (1 << 5)
#define ACCESS_ERROR_1_IRQ    (1 << 6)
#define ACCESS_ERROR_0_IRQ    (1 << 7)

#define ARM_TIMER_IRQ_PENDING         (1 << 0)
#define ARM_MAILBOX_IRQ_PENDING       (1 << 1)
#define ARM_DOORBELL_0_IRQ_PENDING    (1 << 2)
#define ARM_DOORBELL_1_IRQ_PENDING    (1 << 3)
#define GPU_0_HALTED_IRQ_PENDING      (1 << 4)
#define GPU_1_HALTED_IRQ_PENDING      (1 << 5)
#define ACCESS_ERROR_1_IRQ_PENDING    (1 << 6)
#define ACCESS_ERROR_0_IRQ_PENDING    (1 << 7)

#define IRQ_BASIC_PENDING	(PBASE+0x0000B200)
#define IRQ_PENDING_1		(PBASE+0x0000B204)
#define IRQ_PENDING_2		(PBASE+0x0000B208)
#define FIQ_CONTROL			(PBASE+0x0000B20C)
#define ENABLE_IRQS_1		(PBASE+0x0000B210)
#define ENABLE_IRQS_2		(PBASE+0x0000B214)
#define ENABLE_BASIC_IRQS	(PBASE+0x0000B218)
#define DISABLE_IRQS_1		(PBASE+0x0000B21C)
#define DISABLE_IRQS_2		(PBASE+0x0000B220)
#define DISABLE_BASIC_IRQS	(PBASE+0x0000B224)

#endif  /*_P_IRQ_H */
