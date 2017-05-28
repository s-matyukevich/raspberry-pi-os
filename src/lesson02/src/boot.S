.section ".text.boot"

.globl _start
_start:
	mrs x0,mpidr_el1
	and  x0,x0,#0xFF
	cbz x0,master
	b hang

master:
	msr   sctlr_el1, xzr             // Disable MMU at EL1

	mov   x0, xzr
	orr   x0, x0, #(1 << 31)         // 64-bit EL1
	msr	hcr_el2, x0

	mov	x0, #0x33ff
	msr	cptr_el2, x0			// Disable copro. traps to EL2

	mov   x0, xzr
	orr   x0, x0, #(1 << 10)         // .RW = 0b1  -->  EL1 is AArch64
	orr   x0, x0, #1                 // .NS = 0b1  -->  Non-secure state
	msr   scr_el3, x0

	mov   x0, xzr
	orr   x0, x0, #(7 << 6)          // .A = .I = .F = 0b1  -->  SErrors, IRQs, and FIQs will all be masked
	//orr   x0, x0, #(0 << 4)          // .M[4] = 0b0  -->  Return to AArch64 state
	orr   x0, x0, #(1 << 2)          // .M[3:1] = 0b100  -->  Return to EL
	orr   x0, x0, #(1 << 0)          // .M[0] = 0b1  -->  Use EL2's dedicated stack pointer
	msr   spsr_el3, x0

	adrp  x0, entry              // Program EL2 entrypoint
	add   x0, x0, :lo12:entry
	msr   elr_el3, x0

	eret                             // Perform exception return to EL2

entry:
	mov sp,#0x08000000

	mov	x0, #3 << 20
	msr	cpacr_el1, x0			// Enable FP/ASIMD

	bl kernel_main
	

hang: b hang

.globl GET_EL
GET_EL:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

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
