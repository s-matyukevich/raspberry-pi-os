#include "arm/mmu.h"
#include "arm/sysregs.h"

#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master			// Hang for all non-primary CPU
	b	proc_hang

proc_hang: 
	b proc_hang

master:
	mov	x0, #SCTLR_MMU_DISABLED
	msr	sctlr_el1, x0		

	mov	x0, #SCR_SECURE_64BIT
	msr	scr_el3, x0

	mov	x0, #SPSR_VALUE
	msr	spsr_el3, x0

	adr	x0, el1_entry		
	msr	elr_el3, x0

	eret				

el1_entry:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	mov	sp, #(2 * SECTION_SIZE) 
	bl	kernel_main
