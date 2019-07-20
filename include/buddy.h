#ifndef BUDDY_H
#define BUDDY_H

#include "list.h"

#define MAX_ORDER 10

struct free_area{
	struct list_head free_list;
	uint32_t nr_free;
};

struct buddy_sys{
	struct free_area free_area[MAX_ORDER + 1];
	uint32_t start_addr;
};

void buddy_sys_init(struct buddy_sys *buddy, uint32_t s_paddr, uint32_t pgs);
struct page_desc *buddy_alloc(struct buddy_sys *buddy, uint32_t order);
void buddy_free(struct buddy_sys *buddy, struct page_desc *pg_desc);

#endif
