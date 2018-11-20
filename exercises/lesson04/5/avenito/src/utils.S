.globl get_el
get_el:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

.globl put32
put32:
	str w1,[x0]
	ret

.globl get32
get32:
	ldr w0,[x0]
	ret

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret

	/* Looking at See AArch64-referenc-manual p.5667 */

.globl generic_timer_init
generic_timer_init:
	mov x0, #1
	msr CNTP_CTL_EL0, x0
	lsl x0, x0, #23 
	msr CNTP_TVAL_EL0, x0
    ret

.globl generic_timer_reset
generic_timer_reset:
    mov x0, #1
	lsl x0, x0, #23 
	msr CNTP_TVAL_EL0, x0
    ret