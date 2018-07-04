#include "utils.h"
#include "printf.h"
#include "timer.h"
#include "entry.h"
#include "peripherals/irq.h"

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

void enable_interrupt_controller()
{
    /** The local interrupt controller can't be enable or disable.
     *  Instead, we are supposed to rely on the interrupt sources
     *  registers to enable or disable their interrupts
     *  
     *  Neverless, we can still change wich core will receive the
     *  interrupts. They go to Core 0 IRQ by default, which is what
     *  we want. This function becomes effectively useless. 
     */
}

void show_invalid_entry_message(int type, unsigned long esr, unsigned long address)
{
    printf("%s, ESR: %x, address: %x\r\n", entry_error_messages[type], esr, address);
}

void handle_irq(void)
{
    // Each Core has its own pending local intrrupts register
    unsigned int irq = get32(CORE0_INTERRUPT_SOURCES);
    switch (irq) {
        case (LTIMER_INTERRUPT):
            handle_local_timer_irq();
            break;
        default:
            printf("Unknown pending irq: %x\r\n", irq);
    }
}