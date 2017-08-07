.globl ret_from_fork
ret_from_fork:
	bl	schedule_tail
	mov	x0, x20
	blr	x19 		//should never return
