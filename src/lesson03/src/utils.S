.globl GET_EL
GET_EL:
	mrs x0, CurrentEL
	lsr x0, x0, #2
	ret

.globl GET_SP
GET_SP:
	mov x0, sp
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
