#ifndef	_MM_H
#define	_MM_H

#define HIGH_MEMORY 			0x3F000000
#define LOW_MEMORY  			0x00400000

#define PAGE_SHIFT	 			12
#define PAGE_SIZE   			(1 << PAGE_SHIFT)	

#define PAGING_MEMORY 			(HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES 			(PAGING_MEMORY/PAGE_SIZE)

unsigned long get_free_page();
void free_page(unsigned long p);
void memzero(unsigned long src, unsigned long n);
void memcpy(unsigned long src, unsigned long dst, unsigned long n);

#endif  /*_MM_H */
