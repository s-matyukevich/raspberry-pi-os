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

.globl set_fpsimd_reg0
set_fpsimd_reg0:
	mov v0.D[0], x0
	ret

.globl set_fpsimd_reg2
set_fpsimd_reg2:
	mov v2.D[0], x0
	ret

.globl set_fpsimd_reg31
set_fpsimd_reg31:
	mov v31.D[0], x0
	ret

.globl get_fpsimd_reg0
get_fpsimd_reg0:
	mov x0, v0.D[0]
	ret

.globl get_fpsimd_reg2
get_fpsimd_reg2:
	mov x0, v2.D[0]
	ret

.globl get_fpsimd_reg31
get_fpsimd_reg31:
	mov x0, v31.D[0]
	ret