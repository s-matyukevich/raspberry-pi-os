.globl GET_EL
GET_EL:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

.globl GET_SP
GET_SP:
	mov x0, sp
	ret

.globl GET_ELR
GET_ELR:
	mrs x0, elr_el1
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

.globl set_pgd
set_pgd:
	msr	ttbr0_el1, x0
	tlbi vmalle1is
  	DSB ISH              // ensure completion of TLB invalidation
	isb
	ret

.globl get_pgd
get_pgd:
	mov x1, 0
	ldr x0, [x1]
	mov x0, 0x1000
	msr	ttbr0_el1, x0
	ldr x0, [x1]
	ret

