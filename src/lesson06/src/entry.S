#include "arm/sysregs.h"
#include "entry.h"
#include "sys.h"

.globl hang
hang: b hang

.macro	ventry	label
.align	7
b	\label
.endm

.macro	kernel_entry, el
sub	sp, sp, #S_FRAME_SIZE
stp	x0, x1, [sp, #16 * 0]
stp	x2, x3, [sp, #16 * 1]
stp	x4, x5, [sp, #16 * 2]
stp	x6, x7, [sp, #16 * 3]
stp	x8, x9, [sp, #16 * 4]
stp	x10, x11, [sp, #16 * 5]
stp	x12, x13, [sp, #16 * 6]
stp	x14, x15, [sp, #16 * 7]
stp	x16, x17, [sp, #16 * 8]
stp	x18, x19, [sp, #16 * 9]
stp	x20, x21, [sp, #16 * 10]
stp	x22, x23, [sp, #16 * 11]
stp	x24, x25, [sp, #16 * 12]
stp	x26, x27, [sp, #16 * 13]
stp	x28, x29, [sp, #16 * 14]

.if	\el == 0
mrs	x21, sp_el0
.else
add	x21, sp, #S_FRAME_SIZE
.endif /* \el == 0 */

mrs	x22, elr_el1
mrs	x23, spsr_el1

stp	x30, x21, [sp, #16 * 15] 
stp	x22, x23, [sp, #16 * 16]
.endm

.macro	kernel_exit, el
ldp	x22, x23, [sp, #16 * 16]
ldp	x30, x21, [sp, #16 * 15] 

.if	\el == 0
msr	sp_el0, x21
.endif /* \el == 0 */

msr	elr_el1, x22			
msr	spsr_el1, x23


ldp	x0, x1, [sp, #16 * 0]
ldp	x2, x3, [sp, #16 * 1]
ldp	x4, x5, [sp, #16 * 2]
ldp	x6, x7, [sp, #16 * 3]
ldp	x8, x9, [sp, #16 * 4]
ldp	x10, x11, [sp, #16 * 5]
ldp	x12, x13, [sp, #16 * 6]
ldp	x14, x15, [sp, #16 * 7]
ldp	x16, x17, [sp, #16 * 8]
ldp	x18, x19, [sp, #16 * 9]
ldp	x20, x21, [sp, #16 * 10]
ldp	x22, x23, [sp, #16 * 11]
ldp	x24, x25, [sp, #16 * 12]
ldp	x26, x27, [sp, #16 * 13]
ldp	x28, x29, [sp, #16 * 14]
add	sp, sp, #S_FRAME_SIZE		
eret
.endm


/*
 * Exception vectors.
 */
.align	11
.globl vectors 
vectors:
	ventry	el1t_invalid	// Synchronous EL1t
	ventry	el1t_invalid	// IRQ EL1t
	ventry	el1t_invalid	// FIQ EL1t
	ventry	el1t_invalid	// Error EL1t

	ventry	el1_sync	// Synchronous EL1h
	ventry	el1_irq			// IRQ EL1h
	ventry	invalid_irq_1	// FIQ EL1h
	ventry	el1_error	// Error EL1h

	ventry	el0_sync		// Synchronous 64-bit EL0
	ventry	el0_irq			// IRQ 64-bit EL0
	ventry	invalid_irq_0	// FIQ 64-bit EL0
	ventry	el0_error	// Error 64-bit EL0

	ventry	el032_invalid	// Synchronous 32-bit EL0
	ventry	el032_invalid	// IRQ 32-bit EL0
	ventry	el032_invalid	// FIQ 32-bit EL0
	ventry	el032_invalid	// Error 32-bit EL0

el1t_invalid:
	kernel_entry 1
	mrs	x0, esr_el1
	bl	show_invalid_el1t_message
	b	hang

el032_invalid:
	kernel_entry 1
	mrs	x0, esr_el1
	bl	show_invalid_el032_message
	b	hang

el1_irq:
	kernel_entry 1 
	bl	handle_irq
	kernel_exit 1 

el0_irq:
	kernel_entry 0 
	bl	handle_irq
	kernel_exit 0 

el1_error:
	kernel_entry 1
	mov 	x0, 1
	mrs	x1, esr_el1
	bl	show_invalid_error_message
	b	hang

el0_error:
	kernel_entry 0
	mov 	x0, 0
	mrs	x1, esr_el1
	bl	show_invalid_error_message
	b	hang

el1_sync:
	kernel_entry 1
	mov 	x0, 1
	mrs	x1, elr_el1
	mrs	x2, esr_el1				// read the syndrome register
	bl	show_invalid_sync_message
	b	hang

el0_sync:
	kernel_entry 0
	mrs	x25, esr_el1				// read the syndrome register
	lsr	x24, x25, #ESR_ELx_EC_SHIFT	// exception class
	cmp	x24, #ESR_ELx_EC_SVC64		// SVC in 64-bit state
	b.eq	el0_svc
	cmp	x24, #ESR_ELx_EC_DABT_LOW	// data abort in EL0
	b.eq	el0_da
	mov 	x0, 1
	mrs	x1, elr_el1
	mrs	x2, esr_el1				// read the syndrome register
	bl	show_invalid_sync_message
	b	hang

sc_nr	.req	x25		// number of system calls
scno	.req	x26		// syscall number
stbl	.req	x27		// syscall table pointer

el0_svc:
	adr	stbl, sys_call_table				// load syscall table pointer
	uxtw	scno, w8						// syscall number in w8
	mov	sc_nr, #__NR_syscalls
	bl	enable_processor_interrupts
	cmp     scno, sc_nr                     // check upper syscall limit
	b.hs	ni_sys

	ldr	x16, [stbl, scno, lsl #3]	// address in the syscall table
	blr	x16				// call sys_* routine
	b	ret_from_syscall
ni_sys:
	bl	show_invalid_syscall_message
	b	hang
ret_from_syscall:
	bl	disable_processor_interrupts		// disable interrupts
	str	x0, [sp, #S_X0]			// returned x0
	kernel_exit 0

el0_da:
	bl	enable_processor_interrupts
	mrs	x0, far_el1
	mrs	x1, esr_el1			
	bl	do_mem_abort
	cmp x0, 0
	b.eq 1f
	bl	show_invalid_data_message
	b	hang
1:
	bl disable_processor_interrupts				
	kernel_exit 0

.globl ret_from_fork
ret_from_fork:
	bl	schedule_tail
	cbz	x19, ret_to_user				// not a kernel thread
	mov	x0, x20
	blr	x19
ret_to_user:
	bl disable_processor_interrupts				
	kernel_exit 0 

invalid_irq_0:
	kernel_entry 0 
	bl	show_invalid_irq_message
	b	hang

invalid_irq_1:
	kernel_entry 1 
	bl	show_invalid_irq_message
	b	hang

.globl irq_vector_init
irq_vector_init:
	adr	x0, vectors		// load VBAR_EL1 with virtual
	msr	vbar_el1, x0		// vector table address
	ret

.globl enable_processor_interrupts
enable_processor_interrupts:
	msr    daifclr, #2 
	ret

.globl disable_processor_interrupts
disable_processor_interrupts:
	msr	daifset, #2
	ret
