#include "mm.h"

.globl memzero
memzero:
	str xzr, [x0], #8
	subs x1, x1, #8
	b.gt memzero
	ret

.global init_memory
init_memory:
	adr x0, bss_begin
	adr x1, bss_end
	sub x1, x1, x0
	b memzero
	ret

.global get_core_id
get_core_id:
	mrs x0, mpidr_el1
	and x0, x0, #0xFF
	ret

.global init_stack
init_stack:
	mov x1, #STACK_OFFSET
	mul x1, x1,x0
	add x1, x1,#LOW_MEMORY
	mov sp, x1
	ret
