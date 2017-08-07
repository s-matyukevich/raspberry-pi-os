.set S_FRAME_SIZE, 272 		// size of all saved registers 

.macro	ventry	label
.align	7
b	\label
.endm

.macro	kernel_entry
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

mrs	x22, elr_el1
mrs	x23, spsr_el1

stp	x30, x22, [sp, #16 * 15] 
str	x23, [sp, #16 * 16]
.endm

.macro	kernel_exit
ldr	x23, [sp, #16 * 16]
ldp	x30, x22, [sp, #16 * 15] 

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
.endm


/*
 * Exception vectors.
 */
.align	11
.globl vectors 
vectors:
	ventry	invalid_irq		// Synchronous EL1t
	ventry	invalid_irq		// IRQ EL1t
	ventry	invalid_irq		// FIQ EL1t
	ventry	invalid_irq		// Error EL1t

	ventry	invalid_irq		// Synchronous EL1h
	ventry	el1_irq			// IRQ EL1h
	ventry	invalid_irq		// FIQ EL1h
	ventry	invalid_irq		// Error EL1h

	ventry	invalid_irq		// Synchronous 64-bit EL0
	ventry	invalid_irq		// IRQ 64-bit EL0
	ventry	invalid_irq		// FIQ 64-bit EL0
	ventry	invalid_irq		// Error 64-bit EL0

	ventry	invalid_irq		// Synchronous 32-bit EL0
	ventry	invalid_irq		// IRQ 32-bit EL0
	ventry	invalid_irq		// FIQ 32-bit EL0
	ventry	invalid_irq		// Error 32-bit EL0


el1_irq:
	kernel_entry 
	bl	handle_irq
	kernel_exit 
	eret					

invalid_irq:
	kernel_entry 
	bl	show_invalid_irq_message
	kernel_exit 
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
