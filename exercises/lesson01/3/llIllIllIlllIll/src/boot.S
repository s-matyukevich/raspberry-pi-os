#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	non_master

proc_hang: 
	b 	proc_hang

	// non-master sp = LOW_MEMORY + processor_id * SECTION_SIZE
non_master:
	mov	x1, #SECTION_SIZE
	mul	x1, x1, x0
	add	x1, x1, #LOW_MEMORY
	mov 	sp, x1
	// wait
	mov	x8, #0x10000
	// processor*0x10000 cycles
	mul	x9, x8, x0
	mov 	x0, x9
	bl 	delay
	mrs	x0, mpidr_el1
	and	x0, x0, #0xFF

	bl 	kernel_main
	b 	proc_hang		// should never come here

master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero
	
	mrs	x0, mpidr_el1
	and	x0, x0, #0xFF

	mov	sp, #LOW_MEMORY 
	bl	kernel_main
	b 	proc_hang		// should never come here
