#ifndef	_MM_H
#define	_MM_H

#define HIGH_MEMORY 0x3F000000
#define LOW_MEMORY  0x00400000
#define PAGE_SIZE   0x00001000	

#define PAGING_MEMORY (HIGH_MEMORY - LOW_MEMORY)
#define PAGING_PAGES (PAGING_MEMORY/PAGE_SIZE)

unsigned long get_free_page();

#endif  /*_MM_H */
