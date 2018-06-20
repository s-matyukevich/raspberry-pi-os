    #include "mm.h"

    .section ".text.boot"

    .globl _start
_start:
    mrs     x0, mpidr_el1
    and     x0, x0,#0xFF
    cbz     x0, master
    b       slave

master:
    adr     x0, bss_begin
    adr     x1, bss_end
    sub     x1, x1, x0
    bl      memzero

    mov     sp, #LOW_MEMORY
    mrs     x0, mpidr_el1
    and     x0, x0,#0xFF
    bl      kernel_main

slave:
    mov     sp, #LOW_MEMORY
    mrs     x0, mpidr_el1
	  and     x0, x0,#0xFF
    lsl     x0, x0, #15
    bl      delay
    mrs     x0, mpidr_el1
	  and     x0, x0,#0xFF
    bl      kernel_main
