#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include"stdint.h"
#include"bitmap.h"
#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0xc009a000

struct pool{
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);

#endif
