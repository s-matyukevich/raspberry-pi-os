#include <stddef.h>
#include <stdint.h>

#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "entry.h"
#include "peripherals/irq.h"

void enable_interrupt_controller()
{
	put32(ENABLE_BASIC_IRQS, ARM_TIMER_IRQ);
}

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
	printf("%s, ESR: %x, address: %x\r\n", entry_error_messages[type], esr, address);
}

void handle_irq(void)
{
	uint32_t irq = get32(IRQ_BASIC_PENDING);
	switch (irq) {
		case ARM_TIMER_IRQ_PENDING:
			handle_timer_irq();
			break;
		default:
			printf("Inknown pending irq: %x\r\n", irq);
	}
}
