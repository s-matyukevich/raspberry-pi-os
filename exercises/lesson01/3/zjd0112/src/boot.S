#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, init_mem		// Hang for all non-primary CPU

    mov x0, #100
    bl  delay
	b	kernel_entr

proc_hang: 
	b 	proc_hang

init_mem:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

kernel_entr:
    mrs x0, mpidr_el1       // get processor id again
    and x0, x0, #0xFF       // check processor id
    mov x1, #STACK_SIZE     // move STACK_SIZE to x1
    mul x1, x0, x1          // processor_id * STACK_SIZE
    add x1, x1, #LOW_MEMORY
	mov	sp, x1
	bl	kernel_main
	b 	proc_hang		    // should never come here
