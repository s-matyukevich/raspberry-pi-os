#include <stddef.h>
#include <stdint.h>

#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "peripherals/irq.h"

void enable_interrupt_controller()
{
	PUT32(ENABLE_BASIC_IRQS, ARM_TIMER_IRQ);
}

void show_invalid_irq_message()
{
	printf("Bad irq detected ");
}

void handle_irq(void)
{
	uint32_t irq = GET32(IRQ_BASIC_PENDING);
	switch (irq) {
		case ARM_TIMER_IRQ_PENDING:
			handle_timer_irq();
			break;
		default:
			
			printf("Inknown pending irq: %x", irq);
	}
}
