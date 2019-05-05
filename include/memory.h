#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include"stdint.h"
#include"bitmap.h"
#include"debug.h"
#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0xc009a000
#define VADDR_START 0xc0100000
#define PAGE_DIR_TABLE_POS 0xfffff000

struct pool{
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;
};

struct virtual_addr{
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start;
};

enum pool_flags{
	PF_KERNEL = 1,
	PF_USER = 2
};

extern struct pool kernel_pool, user_pool;
void mem_init(void);

#endif
