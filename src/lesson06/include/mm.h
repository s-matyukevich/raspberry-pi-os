#ifndef	_MM_H
#define	_MM_H

#include "sched.h"

#define VA_START 0xffff000000000000
//#define VA_START 0x0

#define HIGH_MEMORY 0x3F000000
#define LOW_MEMORY  0x00400000
#define PAGE_SIZE   0x00001000	

#define PAGING_MEMORY (HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES (PAGING_MEMORY/PAGE_SIZE)

#define PAGE_MASK			0xfffffffffffff000
#define PAGE_SHIFT	 		12
#define TABLE_SHIFT 		9

#define PTRS_PER_TABLE		(1 << TABLE_SHIFT)

#define PGD_SHIFT			PAGE_SHIFT + 3*TABLE_SHIFT
#define PUD_SHIFT			PAGE_SHIFT + 2*TABLE_SHIFT
#define PMD_SHIFT			PAGE_SHIFT + TABLE_SHIFT

#define MM_TYPE_PAGE_TABLE	0x3

#define MM_TYPE_PAGE_TABLE	0x3
#define MM_TYPE_PAGE 		0x3
#define MM_ACCESS			0x1 << 10
#define MM_ACCESS_PERMISSION			0x01 << 6 

#define MT_NORMAL_NC 		0x1

#define MMU_PTE_FLAGS		(MM_TYPE_PAGE | (MT_NORMAL_NC << 2) | MM_ACCESS | MM_ACCESS_PERMISSION)	


void free_page(unsigned long p);
unsigned long get_free_page();
void map_page(struct task_struct *task, unsigned long va, unsigned long page);
void memzero(unsigned long src, unsigned long n);
void memcpy(unsigned long src, unsigned long dst, unsigned long n);

int copy_virt_memory(struct task_struct *dst); 
unsigned long allocate_kernel_page(); 
unsigned long allocate_user_page(struct task_struct *task, unsigned long va); 

extern unsigned long pg_dir;

#endif  /*_MM_H */
