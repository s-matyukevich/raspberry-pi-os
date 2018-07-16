#include "sched.h"

.globl cpu_switch_to
cpu_switch_to:
	mov	x10, #THREAD_CPU_CONTEXT
	add	x8, x0, x10
	mov	x9, sp
	stp	x19, x20, [x8], #16    // store callee-saved registers
	stp	x21, x22, [x8], #16
	stp	x23, x24, [x8], #16
	stp	x25, x26, [x8], #16
	stp	x27, x28, [x8], #16
	stp	x29, x9, [x8], #16
	str	x30, [x8]
	add	x8, x1, x10
	ldp	x19, x20, [x8], #16    // restore callee-saved registers
	ldp	x21, x22, [x8], #16
	ldp	x23, x24, [x8], #16
	ldp	x25, x26, [x8], #16
	ldp	x27, x28, [x8], #16
	ldp	x29, x9, [x8], #16
	ldr	x30, [x8]
	mov	sp, x9

	mov x10, #THREAD_FPSIMD_CONTEXT
	add x8, x0, x10
	stp q0, q1, [x8], #32      // store FP/SIMD registers
	stp q2, q3, [x8], #32
	stp q4, q5, [x8], #32
	stp q6, q7, [x8], #32
	stp q8, q9, [x8], #32
	stp q10, q11, [x8], #32
	stp q12, q13, [x8], #32
	stp q14, q15, [x8], #32
	stp q16, q17, [x8], #32
	stp q18, q19, [x8], #32
	stp q20, q21, [x8], #32
	stp q22, q23, [x8], #32
	stp q24, q25, [x8], #32
	stp q26, q27, [x8], #32
	stp q28, q29, [x8], #32
	stp q30, q31, [x8], #32
	mrs x11, fpsr			   // store FP/SIMD control & status register 
	str x11, [x8], #8
	mrs x11, fpcr
	str x11, [x8]

	add x8, x1, x10
	ldp q0, q1, [x8], #32      // restore FP/SIMD registers
	ldp q2, q3, [x8], #32
	ldp q4, q5, [x8], #32
	ldp q6, q7, [x8], #32
	ldp q8, q9, [x8], #32
	ldp q10, q11, [x8], #32
	ldp q12, q13, [x8], #32
	ldp q14, q15, [x8], #32
	ldp q16, q17, [x8], #32
	ldp q18, q19, [x8], #32
	ldp q20, q21, [x8], #32
	ldp q22, q23, [x8], #32
	ldp q24, q25, [x8], #32
	ldp q26, q27, [x8], #32
	ldp q28, q29, [x8], #32
	ldp q30, q31, [x8], #32
	ldr x11, [x8], #8          // restore FP/SIMD control & status register
	msr fpsr, x11
	ldr x11, [x8]
	msr fpcr, x11

	ret

