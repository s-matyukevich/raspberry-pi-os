#ifndef	_MM_H
#define	_MM_H

#include "peripherals/base.h"

#define VA_START 			0xffff000000000000

#define PHYS_MEMORY_SIZE 		0x40000000	

#define PAGE_MASK			0xfffffffffffff000
#define PAGE_SHIFT	 		12
#define TABLE_SHIFT 			9
#define SECTION_SHIFT			(PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE   			(1 << PAGE_SHIFT)	
#define SECTION_SIZE			(1 << SECTION_SHIFT)	

#define LOW_MEMORY              	(2 * SECTION_SIZE)
#define HIGH_MEMORY             	DEVICE_BASE

#define PAGING_MEMORY 			(HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES 			(PAGING_MEMORY/PAGE_SIZE)

#define PTRS_PER_TABLE			(1 << TABLE_SHIFT)

#define PGD_SHIFT			PAGE_SHIFT + 3*TABLE_SHIFT
#define PUD_SHIFT			PAGE_SHIFT + 2*TABLE_SHIFT
#define PMD_SHIFT			PAGE_SHIFT + TABLE_SHIFT

#define PG_DIR_SIZE			(3 * PAGE_SIZE)

#ifndef __ASSEMBLER__

#include "sched.h"

unsigned long get_free_page();
void free_page(unsigned long p);
void map_page(struct task_struct *task, unsigned long va, unsigned long page);
void memzero(unsigned long src, unsigned long n);
void memcpy(unsigned long dst, unsigned long src, unsigned long n);

int copy_virt_memory(struct task_struct *dst); 
unsigned long allocate_kernel_page(); 
unsigned long allocate_user_page(struct task_struct *task, unsigned long va); 

extern unsigned long pg_dir;

#endif

#endif  /*_MM_H */
