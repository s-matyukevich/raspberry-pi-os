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

void show_invalid_el1t_message(unsigned long reg)
{
	printf("EL!t error, syndrome register: %x\r\n", reg);
}

void show_invalid_el032_message(unsigned long reg)
{
	printf("EL032 error, syndrome register: %x\r\n", reg);
}

void show_invalid_error_message(int el, unsigned long reg)
{
	printf("Error, syndrome register: %x, exception level: %d\r\n", reg, el);
}

void show_invalid_sync_message(int el, unsigned long reg)
{
	printf("Sync error, syndrome register: %x, exception level: %d\r\n", reg, el);
}

void show_invalid_irq_message()
{
	printf("Bad FIQ detected\r\n ");
}

void show_reg(unsigned long reg)
{
	printf("Reg: %x\r\n", reg);
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
