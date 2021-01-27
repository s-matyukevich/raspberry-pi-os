#include "utils.h"
#include "custom_printf.h"
#include "timer.h"
#include "entry.h"
#include "peripherals/irq.h"
#include "sysregs.h"

const char *entry_error_messages[] = {
	"SYNC_INVALID_EL1t",
	"IRQ_INVALID_EL1t",		
	"FIQ_INVALID_EL1t",		
	"ERROR_INVALID_EL1T",		

	"SYNC_INVALID_EL1h",		
	"IRQ_INVALID_EL1h",		
	"FIQ_INVALID_EL1h",		
	"ERROR_INVALID_EL1h",		

	"SYNC_INVALID_EL0_64",		
	"IRQ_INVALID_EL0_64",		
	"FIQ_INVALID_EL0_64",		
	"ERROR_INVALID_EL0_64",	

	"SYNC_INVALID_EL0_32",		
	"IRQ_INVALID_EL0_32",		
	"FIQ_INVALID_EL0_32",		
	"ERROR_INVALID_EL0_32"	
};

void enable_interrupt(unsigned int irq) {

	// For interrupt ID m, when DIV and MOD are the integer division and modulo operations:
	// • the corresponding GICD_ISENABLER number, n, is given by n = m DIV 32
	// • the offset of the required GICD_ISENABLER is (0x100 + (4*n))
	// • the bit number of the required Set-enable bit in this register is m MOD 32.

	unsigned int n = irq / 32;
	unsigned int offset = irq % 32;
	unsigned int enableRegister = GIC_ENABLE_IRQ_BASE + (4*n);

	put32(enableRegister, 1 << offset);
}

void assign_target(unsigned int irq, unsigned int cpu) {
	// For interrupt ID m, when DIV and MOD are the integer division and modulo operations:
	// • the corresponding GICD_ITARGETSRn number, n, is given by n = m DIV 4
	// • the offset of the required GICD_ITARGETSR is (0x800 + (4*n))
	// • the byte offset of the required Priority field in this register is m MOD 4, where:
	// 		— byte offset 0 refers to register bits [7:0]
	//    	— byte offset 1 refers to register bits [15:8]
	//    	— byte offset 2 refers to register bits [23:16]
	//    	— byte offset 3 refers to register bits [31:24].

	unsigned int n = irq / 4;
	unsigned int offset = irq % 4;
	unsigned int targetRegister = GIC_IRQ_TARGET_BASE + (4*n);

	// Currently we only enter the target CPU 0
	put32(targetRegister, get32(targetRegister) | (1 << (offset*8)));
}


void enable_interrupt_controller() {	
	assign_target(SYSTEM_TIMER_IRQ_1, 0);
	enable_interrupt(SYSTEM_TIMER_IRQ_1);

	assign_target(LOCAL_TIMER_IRQ, 0);
	enable_interrupt(LOCAL_TIMER_IRQ);
}

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
	printf("%s, ESR: %x, address: %x\r\n", entry_error_messages[type], esr, address);
}

void handle_irq(void)
{
	// NOTE (ARM® Generic Interrupt Controller - Architecture version 2.0): 
	// For compatibility with possible extensions to the GIC architecture specification, ARM recommends that software
	// preserves the entire register value read from the GICC_IAR when it acknowledges the interrupt, and uses that entire
	// value for its corresponding write to the GICC_EOIR.
	unsigned int irqAckRegister = get32(GICC_IAR);

	unsigned int irq = irqAckRegister & 0x2FF; // bit [9:0] -> interrupt ID
	switch (irq) {
		case (SYSTEM_TIMER_IRQ_1):
			handle_timer_irq();		
			put32(GICC_EOIR, irqAckRegister);
			break;
		case (LOCAL_TIMER_IRQ): 
			handle_local_timer_irq();
			put32(GICC_EOIR, irqAckRegister);
			break;
		default:
			printf("Unknown pending irq: %x\r\n", irq);
	}

	

}