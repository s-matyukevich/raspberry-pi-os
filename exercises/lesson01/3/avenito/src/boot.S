#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// get processor id
	cbz	x0, master
	b	init_stack

master:						// only master needs to initialize de memory
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

init_stack:
	mrs	x0, mpidr_el1
	and	x0, x0,#0xFF		// get processor id again
	mov x1, x0				// store processor id in 'x1' to preserve 'x0' when call kernel_main
	mov x2, #0x1000			// define the stack offset in 4Kb (0x1000)
	mul x1, x1,x2			// multiply the stack offset by processor id
	add x1, x1,#LOW_MEMORY	// add to base address (#LOW_MEMORY)
	mov	sp, x1
	bl	kernel_main			// call 'kernel_main' passing processor id through 'x0'

proc_hang:
	b 	proc_hang
