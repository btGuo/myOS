#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "debug.h"
#include "sync.h"
#include "list.h"

#define PG_SIZE 4096
#define MEM_BITMAP_BASE 0xc009a000
#define VADDR_START 0xc0100000
#define PAGE_DIR_TABLE_POS 0xfffff000

struct pool{
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;
	struct mutex_lock lock;
};

struct virtual_addr{
	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start;
};

enum pool_flags{
	PF_KERNEL = 1,
	PF_USER = 2
};

struct mem_block_desc{
	uint32_t block_size;
	uint32_t blocks;
	struct list_head free_list;
};

#define DESC_CNT 7
extern struct pool kernel_pool, user_pool;
void mem_init(void);
void *get_kernel_pages(uint32_t pg_cnt);
void *get_user_pages(uint32_t pg_cnt);
void *get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

void block_desc_init(struct mem_block_desc *blk_desc);
typedef struct list_head mem_block;
void *sys_malloc(uint32_t size);

#endif
