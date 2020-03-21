#ifndef	_MM_H
#define	_MM_H

#define PAGE_SHIFT	 		12
#define TABLE_SHIFT 			9
#define SECTION_SHIFT			(PAGE_SHIFT + TABLE_SHIFT)

#define PAGE_SIZE   			(1 << PAGE_SHIFT)	
#define SECTION_SIZE			(1 << SECTION_SHIFT)	

#define LOW_MEMORY              	(2 * SECTION_SIZE)

#define STACK_OFFSET			1 << 10 // 1KB

#ifndef __ASSEMBLER__

void memzero(unsigned long src, unsigned long n);

void init_memory(unsigned long begin, unsigned long end);
void init_stack(unsigned int cort_id);
int get_core_id();

#endif

#endif  /*_MM_H */
