.section ".text.boot"

.globl _start
_start:
	mrs x0,mpidr_el1
	and  x0,x0,#0xFF
	cbz x0,master
	b hang

master:
	mov sp,#0x08000000
	bl kernel_main

hang: b hang

.globl PUT32
PUT32:
	str w1,[x0]
	ret

.globl GET32
GET32:
	ldr w0,[x0]
	ret

.globl DELAY
DELAY:
	subs x0, x0, #1
	bne DELAY
	ret
