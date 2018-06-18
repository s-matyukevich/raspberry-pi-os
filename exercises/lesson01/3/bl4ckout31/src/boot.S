#include "mm.h"

.data
barrier: .byte 0
         .align 2

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, bss_init		// CPU #0 do BSS initialization, others wait
	b wait

bss_init:
	// BSS Initialization
    adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero
	
	// Tell other CPU that BSS is ready
	mov w0, #1
	ldr x1, =barrier
	strb w0, [x1]
	b master

wait:
	// Wait for BSS to bo initialized
	ldr x1, =barrier
	ldrb w0, [x1]
	cmp w0, #1
	beq master
	b wait

proc_hang: 
	b 	proc_hang

master:
	// Use the CPU ID to determine the stack location: 
	// CPU #0: sp = 1 * #LOW_MEMORY, CPU #1: sp = 2 * #LOW_MEMORY, etc
	mov	x0, #LOW_MEMORY 
	mrs x1, mpidr_el1
	and x1, x1, #0xFF
	add x1, x1, #1
	mul x0, x0, x1
	mov sp, x0
	
	bl	kernel_main
	b 	proc_hang
