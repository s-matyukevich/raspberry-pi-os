#include "mm.h"

static unsigned short mem_map[PAGING_PAGES] = {
    0,
};

unsigned long get_free_page() {
  for (int i = 0; i < PAGING_PAGES; i++) {
    if (mem_map[i] == 0) {
      mem_map[i] = 1;
      return LOW_MEMORY + i * PAGE_SIZE;
    }
  }
  return 0;
}

void free_page(unsigned long p) { mem_map[(p - LOW_MEMORY) / PAGE_SIZE] = 0; }
