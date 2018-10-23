# How I figured out that QEMU start at EL2
First I did the changes to use UART instead mini_UART, but the simulation didn't work.
After a few tries, I removed the folowing code from boot.S:

	ldr	x0, =SCTLR_VALUE_MMU_DISABLED
	msr	sctlr_el1, x0		

	ldr	x0, =HCR_VALUE
	msr	hcr_el2, x0

	ldr	x0, =SCR_VALUE
	msr	scr_el3, x0
	
	ldr	x0, =SPSR_VALUE
	msr	spsr_el2, x0

	adr	x0, el1_entry		
	msr	elr_el2, x0

	eret
	
The simulation worked and the output was: "Exception level: 2". That means, the qemu starts at EL2

