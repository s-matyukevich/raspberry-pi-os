#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	mrs	x0, mpidr_el1
	and	x0, x0, #0x3
	cbz	x0, init_bss
	/* If processor id is not 0 then pending lock processor
	 * (wait for `sev` instruction)
	 */
	wfe
	b	master

proc_hang:
	b 	proc_hang

init_bss:
	adr	x0, bss_begin
	adr	x1, bss_end
	sub	x1, x1, x0
	bl 	memzero

	sev
	b master

master:
	mrs	x0, mpidr_el1
	and	x0, x0, #0x3

	mov	x1, #SECTION_SIZE
	mul	x1, x1, x0
	add	x1, x1, #LOW_MEMORY
	mov	sp, x1

	bl	kernel_main
	b 	proc_hang		// should never come here
