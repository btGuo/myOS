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
#define MAX_PG_CNT (1 << 20)
#define DESC_CNT 7


/**
 * 页描述符，每个物理页有一个
 */
struct page_desc{
	struct list_head cache_tag; ///< 缓冲用
	uint32_t order;      ///< 用于伙伴系统，当前页框的阶
};

struct pool{
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;
	struct mutex_lock lock;
};

struct virtual_addr{
//	struct bitmap vaddr_bitmap;
	uint32_t vaddr_start;
};

enum pool_flags{
	PF_KERNEL,
	PF_USER
};

struct mem_block_desc{
	uint32_t block_size;
	uint32_t blocks;
	struct list_head free_list;
};

typedef struct list_head mem_block;

extern struct pool kernel_pool, user_pool;
void mem_init(void);
void *get_kernel_pages(uint32_t pg_cnt);
void *get_user_pages(uint32_t pg_cnt);
void *get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

void block_desc_init(struct mem_block_desc *blk_desc);

void *sys_malloc(uint32_t size);
void sys_free(void *ptr);
void *kmalloc(uint32_t size);
void kfree(void *ptr);

#endif
