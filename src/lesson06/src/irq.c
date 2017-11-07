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

void show_invalid_sync_message(int el, unsigned long address, unsigned long reg)
{
	printf("ttbr0_el1: %x\n\r", get_pgd());
	printf("Sync error, syndrome register: %x, address: %x, exception level: %d\r\n", reg, address, el);
}

void show_invalid_irq_message()
{
	printf("Bad FIQ detected\r\n ");
}

void show_invalid_syscall_message()
{
	printf("Bad syscall detected ");
}

void show_invalid_data_message()
{
	printf("Data exception not handled.");
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
