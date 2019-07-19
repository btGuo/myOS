#ifndef BUDDY_H
#define BUDDY_H

#include "list.h"

#define MAX_ORDER 11

struct free_area{
	struct list_head free_list;
	uint32_t nr_free;
};

struct buddy_sys{
	struct free_area free_area[MAX_ORDER + 1];
	uint32_t start_addr;
};

typedef struct list_head area_head;

#endif
