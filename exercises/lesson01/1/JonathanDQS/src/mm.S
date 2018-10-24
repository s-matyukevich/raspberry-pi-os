.globl memzero
memzero:
//XZR zero register for store instructions rather than move 0 to a known register
	str xzr, [x0], #8
//x1 holds the size to be written with 0s so update until all have been done
	subs x1, x1, #8
//Loop if not finished ; QUESTION why the dot?
	b.gt memzero
	ret
