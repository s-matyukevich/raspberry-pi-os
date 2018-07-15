#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	setup_stack

master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

setup_stack:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
    mov x1, 0x4000          // 2Kb for stack
	mul x1, x1, x0 			// multiply 2kb by processor id
    mov x2, #LOW_MEMORY		// Start of stack stpace
	add x1, x1, x2			// add our offset
	mov	sp, x1 
	bl	kernel_main
	b 	proc_hang		// should never come here

proc_hang: 
	b 	proc_hang
