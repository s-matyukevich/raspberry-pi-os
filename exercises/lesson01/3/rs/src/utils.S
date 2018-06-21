.globl put32
put32:
	str w1, [x0]
	ret

.globl get32
get32:
	ldr w0, [x0]
	ret

.globl delay
delay:
	subs x0, x0, #1
	bne delay
	ret

.globl getmpid
getmpid:
	mrs x0, mpidr_el1
	and x0, x0, #0xFF
	ret
