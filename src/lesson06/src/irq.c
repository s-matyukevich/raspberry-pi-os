#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "entry.h"
#include "peripherals/irq.h"

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1t",		
	"SYNC_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	
	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32",	
	"SYNC_ERROR",				
	"SYSCALL_ERROR",			
	"DATA_ABORT_ERROR"		
};

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
	unsigned int irq = get32(IRQ_BASIC_PENDING);
	switch (irq) {
		case ARM_TIMER_IRQ_PENDING:
			handle_timer_irq();
			break;
		default:
			printf("Inknown pending irq: %x\r\n", irq);
	}
}
