#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
    cbz x0, master
	b   init_stack		// Hang for all non-primary CPU

proc_hang: 
	b 	proc_hang

master:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero
init_stack:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
    mov x2, #0x1000
    mul x1, x0,x2      //4kb for stack size    
    add x1, x1,#LOW_MEMORY  //offset for stack
	mov	sp, x1
	bl	kernel_main
	b 	proc_hang		// should never come here
