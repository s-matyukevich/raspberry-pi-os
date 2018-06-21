#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	// Only proc 0 init BSS.
	mrs x0, mpidr_el1
	and x0, x0, #0xFF
	cbz	x0, bss_init

	// Other procs are delayed to let proc 0 init BSS.
	mov x0, #100
	bl delay
	b master

bss_init:
	adr x0, bss_begin
	adr x1, bss_end
	sub x1, x1, x0
	bl memzero
	b master

proc_hang: 
	b proc_hang

master:
	// Init stack with each core its own section.
	// First core starts at #LOW_MEMORY and other core at 
	// #LOW_MEMORY + (core_id * #SECTION_SIZE).
	mrs x0, mpidr_el1
	and x0, x0, #0xFF
	mov x1, #LOW_MEMORY
	mov x2, #SECTION_SIZE
	mul x2, x2, x0
	add x1, x1, x2
	mov sp, x1

	bl kernel_main
	b proc_hang
