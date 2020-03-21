#include "mm.h"

.section ".text.boot"

.globl _start
_start:
	b	master

proc_hang: 
	b 	proc_hang

master:
	bl 	get_core_id
	cbz	x0, init_memory
	bl	get_core_id
	bl	init_stack
	bl	get_core_id
	bl	kernel_main
