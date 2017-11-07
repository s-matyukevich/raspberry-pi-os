.section ".text.boot"

.set VA_START,			0xffff000000000000
.set DEVICE_BASE, 		0x3F000000	
.set DEV_TEST,			0x00215054
.set PHYS_MEMORY_SIZE, 		0x40000000	
.set PAGE_SIZE,			0x00001000

.set PAGE_SHIFT,		12
.set TABLE_SHIFT,		9
.set VA_BITS,			(PAGE_SHIFT + 4*TABLE_SHIFT)

.set PGDIR_SHIFT,		(PAGE_SHIFT + 3*TABLE_SHIFT)
.set PUD_SHIFT,			(PAGE_SHIFT + 2*TABLE_SHIFT)
.set SECTION_SHIFT,		(PAGE_SHIFT + TABLE_SHIFT)

.set SECTION_SIZE,		(1 << SECTION_SHIFT)	

.set PTRS_PER_TABLE,		(1 << TABLE_SHIFT)

.set PG_DIR_SIZE,		(4 * PAGE_SIZE)

.set MM_TYPE_PAGE_TABLE,	0x3
.set MM_TYPE_BLOCK, 		0x1
.set MM_ACCESS,			(0x1 << 10)

.set MT_DEVICE_nGnRnE, 		0x0
.set MT_NORMAL_NC, 		0x1

.set MMU_FLAGS,			(MM_TYPE_BLOCK | (MT_NORMAL_NC << 2) | MM_ACCESS)	
.set MMU_DEVICE_FLAGS,		(MM_TYPE_BLOCK | (MT_DEVICE_nGnRnE << 2) | MM_ACCESS)	

.set TCR_T0SZ,			(64 - 48) 
.set TCR_T1SZ,			((64 - 48) << 16)
.set TCR_TG0_4K,		(0 << 14)
.set TCR_TG1_4K,		(2 << 30)
.set TCR_VALUE,			(TCR_T0SZ | TCR_T1SZ | TCR_TG0_4K | TCR_TG1_4K)

.globl _start
_start:
	mrs	x0, mpidr_el1		
	and	x0, x0,#0xFF		// Check processor id
	cbz	x0, master		// Hang for all non-primary CPU
	b	hang

hang: b hang

master:
	msr	sctlr_el1, xzr		// Disable MMU at EL1

	mov	x0, xzr
	orr	x0, x0, #(1 << 31)	// 64-bit EL1
	msr	hcr_el2, x0

	mov	x0, #0x33ff
	msr	cptr_el2, x0		// Disable copro. traps to EL2

	mov	x0, xzr
	orr	x0, x0, #(1 << 10)	// .RW = 0b1  -->  EL1 is AArch64
//	orr	x0, x0, #1		// .NS = 0b1  -->  Non-secure state
	msr	scr_el3, x0

	mov	x0, xzr
	orr	x0, x0, #(7 << 6)	// .A = .I = .F = 0b1  -->  SErrors, IRQs, and FIQs will all be masked
	orr	x0, x0, #(1 << 2)	// .M[3:1] = 0b100  -->  Return to EL
	orr	x0, x0, #(1 << 0)	// .M[0] = 0b1  -->  Use EL1's dedicated stack pointer
	msr	spsr_el3, x0

	adr	x0, el1_entry		// Program EL1 entrypoint
	msr	elr_el3, x0

	eret				// Perform exception return to EL1

el1_entry:
	adrp	x7, pg_dir
	mov	x8, #VA_START
	bl 	__create_page_tables

//	adrp	x7, pg1_dir
//	mov	x8, #VA1_START
//	bl 	__create_page_tables

	mov	x0, #VA_START
	add	sp, x0, #(2 * SECTION_SIZE)

	//adrp	x0, pg_dir			// set page table address 
	//msr	ttbr0_el1, x0

	adrp	x0, pg_dir			// set page table address 
	msr	ttbr1_el1, x0

	ldr	x0, =TCR_VALUE			// set Translation Control Register
	msr	tcr_el1, x0

	/*
	 * Memory region attributes for LPAE:
	 *
	 *   n = AttrIndx[2:0]
	 *			n	MAIR
	 *   DEVICE_nGnRnE	000	00000000
	 *   NORMAL_NC		001	01000100
	 */
	ldr	x0, =(0x00 << (8 * MT_DEVICE_nGnRnE)) | (0x44 << (8 * MT_NORMAL_NC))
	msr	mair_el1, x0

	
	ldr	x2, =kernel_main

	mov	x0, 0x1				// enable MMU
	msr	sctlr_el1, x0


	br 	x2


	.macro	create_pgd_entry, tbl, virt, tmp1, tmp2
	create_table_entry \tbl, \virt, PGDIR_SHIFT, \tmp1, \tmp2
	create_table_entry \tbl, \virt, PUD_SHIFT, \tmp1, \tmp2
	.endm

	.macro	create_table_entry, tbl, virt, shift, tmp1, tmp2
	lsr	\tmp1, \virt, #\shift
	and	\tmp1, \tmp1, #PTRS_PER_TABLE - 1	// table index
	add	\tmp2, \tbl, #PAGE_SIZE
	orr	\tmp2, \tmp2, #MM_TYPE_PAGE_TABLE	
	str	\tmp2, [\tbl, \tmp1, lsl #3]
	add	\tbl, \tbl, #PAGE_SIZE			// next level table page
	.endm

	.macro	create_block_map, tbl, phys, start, end, flags, tmp1
	lsr	\phys, \phys, #SECTION_SHIFT
	lsr	\start, \start, #SECTION_SHIFT
	and	\start, \start, #PTRS_PER_TABLE - 1	// table index
	mov	\tmp1, #\flags
	orr	\phys, \tmp1, \phys, lsl #SECTION_SHIFT	// table entry
	lsr	\end, \end, #SECTION_SHIFT
	and	\end, \end, #PTRS_PER_TABLE - 1		// table end index
9999:	str	\phys, [\tbl, \start, lsl #3]		// store the entry
	add	\start, \start, #1			// next entry
	add	\phys, \phys, #SECTION_SIZE		// next block
	cmp	\start, \end
	b.ls	9999b
	.endm

__create_page_tables:
	// x7 page_dir
	// x8 VA_START 

//	adrp	x0, pg_dir
	mov 	x0, x7
	mov	x1, #PG_DIR_SIZE
1:	stp	xzr, xzr, [x0], #16
	subs	x1, x1, #16
	b.ne	1b

	mov 	x0, x7
	mov	x1, x8
//adrp	x0, pg_dir
//mov	x1, #VA_START 
	create_pgd_entry x0, x1, x2, x3

	/* Mapping kernel and init stack*/
	mov 	x1, xzr					// start mapping from physical offset 0
	mov	x2, x8
//	mov 	x2, #VA_START				// first virtual address
	mov	x3, x8
	mov	x5, #(DEVICE_BASE - SECTION_SIZE)
	add	x3, x3, x5				// last virtual address
	create_block_map x0, x1, x2, x3, MMU_FLAGS, x4

	/* Mapping device memory*/
	mov 	x1, #DEVICE_BASE			// start mapping from device base address 
	mov	x2, x8
	mov	x5, #(DEVICE_BASE)
	add	x2, x2, x5 
//	mov 	x2, #(VA_START + DEVICE_BASE)				// first virtual address
	mov	x3, x8
	mov 	x5, #(PHYS_MEMORY_SIZE - SECTION_SIZE)
	add	x3, x3, x5
//	mov	x3, #(VA_START + PHYS_MEMORY_SIZE - SECTION_SIZE)				// last virtual address
	create_block_map x0, x1, x2, x3, MMU_DEVICE_FLAGS, x4
	ret
