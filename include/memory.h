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
#define PAGE_DIR_TABLE_POS 0xc0100000   ///< 内核页目录位置
#define MAX_PG_CNT (1 << 20)
#define DESC_CNT 7
#define K_VADDR_START 0xc0000000

#define ALLOC_MASK 0x80000000

#define PAGE_ALLOC(pg_desc) (((pg_desc)->order) &= ~ALLOC_MASK)   ///< 分配为0
#define PAGE_FREE(pg_desc) (((pg_desc)->order) |= ALLOC_MASK)    ///< 未分配为1
#define IS_PAGE_FREE(pg_desc) (((pg_desc)->order) & ALLOC_MASK)

/**
 * 页描述符，每个物理页有一个
 */
struct page_desc{
	struct list_head cache_tag; ///< 缓冲用
	uint16_t count;      ///< 页引用计数，共享内存时用
	uint8_t order;      ///< 用于伙伴系统，当前页框的阶，最高位被编码为是否分配
	uint8_t free;       ///< 页是否分配出去
};

struct pool{
	struct bitmap pool_bitmap;
	uint32_t phy_addr_start;
	uint32_t pool_size;
	struct mutex_lock lock;
};

struct umem_manager{
	struct list_head page_caches;
	struct page_desc *page_table;
	uint32_t paddr_start;
	uint32_t pages;
	uint32_t free_pages;
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

struct kmem_manager{
	struct buddy_sys buddy;
	struct page_desc *page_table; ///< 内核页框描述符表
	struct mutex_lock lock;
	struct list_head page_caches; 
	struct mem_block_desc block_desc[DESC_CNT];
	uint32_t pg_cahces_cnt;
	uint32_t paddr_start;
	uint32_t vaddr_start;
	uint32_t pages;
	uint32_t buddy_paddr_start;
};

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
void copy_page_table(uint32_t dest_saddr, uint32_t src_saddr);

#endif
