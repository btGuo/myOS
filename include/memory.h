#ifndef __KERNEL_MEMORY_H
#define __KERNEL_MEMORY_H
#include "stdint.h"
#include "bitmap.h"
#include "sync.h"
#include "list.h"
#include "buddy.h"
#include "vm_area.h"
#include "debug.h"

#define PG_SIZE 4096

#define MAX_PG_CNT (1 << 20)
#define DESC_CNT 7
#define K_VADDR_START 0xc0000000
#define VADDR_OFF (0xc0000000)


// 物理内存布局
/************************************************************************************************
 *
 * 0-------1M ------2M--------3M-------------------------------4nM------------4nM------------end
 * |        |        |         |                |              |                |              |
 * |低端内存|内核镜像|内核页表 |所有物理页描述符|零散物理页    |内核固定映射内|用户内存区和内核|
 * |不用    |        |         |kmm.page_table  |kmm.page_cache|存伙伴系统管理|动态映射物理页  |
 * |        |        |         |                |              |                |              |
 * ---------------------------------------------------------------------------------------------
 *
 **************************************************************************************************/

/**
 * 页描述符，每个物理页有一个
 */
struct page_desc{
	struct list_head cache_tag; ///< 缓冲用
	uint16_t count;      ///< 页引用计数，共享内存时用
	uint8_t order;      ///< 用于伙伴系统，当前页框的阶
	uint8_t free;       ///< 页是否分配出去
#ifdef DEBUG
	uint32_t pos;     ///< debug用，当前页框编号
#endif
};

/**
 * 内存池flag
 */
enum pool_flags{
	PF_KERNEL, ///< 内核
	PF_USER    ///< 用户
};

struct mem_block_desc{
	uint32_t block_size;
	uint32_t blocks;
	struct list_head free_list;
};

typedef struct list_head mem_block;

/**
 * 内核动态内存区
 */
struct v_area{
	uint32_t s_addr;   ///< 起始地址
	uint32_t pg_cnt;   ///< 页数
	struct list_head list_tag; ///< 链表节点
};

//元数据块，表示两种内存，以页为单位分配时，desc为null;
struct meta{
	struct mem_block_desc *desc;
	uint32_t cnt;
};
/**
 * 内核内存管理描述符
 */
struct kmem_manager{
	struct buddy_sys buddy;     ///< 伙伴系统
	struct page_desc *page_table; ///< 内核页框描述符表
	struct mutex_lock lock;     ///< 锁
	struct list_head page_caches;   ///< 缓冲页，零散物理页会被加入这里，可能为空
	struct mem_block_desc block_desc[DESC_CNT]; ///< 分配小块内存用
	struct list_head v_area_head;   ///< 动态内存区链表
	uint32_t pg_cahces_cnt;      ///< 对应page_caches 物理页数
	uint32_t paddr_start;       ///< 起始物理地址
	uint32_t vaddr_start;       ///< 内核起始虚拟地址
	uint32_t v_area_saddr;      ///< 动态内存区起始地址
	uint32_t pages;             ///< 内核物理页数
	uint32_t buddy_paddr_start; ///< 伙伴系统起始物理地址
};

/**
 * 用户内存管理描述符
 */
struct umem_manager{
	struct list_head page_caches;  ///< 页缓存
	struct mutex_lock lock;       ///< 锁
	struct page_desc *page_table; ///< 页描述符首地址
	uint32_t paddr_start;   ///< 起始物理地址
	uint32_t pages;         ///< 总物理页数
	uint32_t free_pages;    ///< 未分配物理页数
};


extern struct kmem_manager kmm;
extern struct umem_manager umm;

/**
 * 页描述符找到页物理地址
 */
static inline uint32_t pgdesc_to_paddr(struct page_desc *pg_desc){
	return (pg_desc - kmm.page_table) * PG_SIZE;
}

/**
 * 页物理地址找到页描述符
 */
static inline struct page_desc *paddr_to_pgdesc(uint32_t paddr){
	return &kmm.page_table[(paddr >> 12)];
}

/**
 * 虚拟地址找到页表项指针
 */
static inline uint32_t *PTE_PTR(uint32_t vaddr){
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + \
			((vaddr & 0x003ff000) >> 10));
}

/**
 * 虚拟地址找到页目录项指针
 */
static inline uint32_t *PDE_PTR(uint32_t vaddr){
	return (uint32_t*)(0xfffff000 + ((vaddr & 0xffc00000) >> 20));
}


void mem_init(void);
void *get_a_page(enum pool_flags pf, uint32_t vaddr);
uint32_t addr_v2p(uint32_t vaddr);

void block_desc_init(struct mem_block_desc *blk_desc);

void *sys_malloc(uint32_t size);
void sys_free(void *ptr);
void copy_page_table(uint32_t *pde);

void *get_kernel_pages(uint32_t pg_cnt);
void free_kernel_pages(void *vaddr);
void *kmalloc(uint32_t size);
void kfree(void *ptr);
void *vmalloc(uint32_t pg_cnt);
void vfree(void *vaddr);

void do_wp_page(uint32_t _vaddr);
void do_page_fault(uint32_t vaddr);
#endif
