#include <memory.h>
#include <stdint.h>
#include <kernelio.h>
#include <string.h>
#include <thread.h>
#include <interrupt.h>
#include <buddy.h>
#include <sync.h>
#include <process.h>
#include <multiboot.h>
/**
 * 内核物理内存固定映射，由伙伴系统统一管理，用户物理内存动态映射，自由链表管理
 */

struct kmem_manager kmm;
struct umem_manager umm;
uint32_t max_mm;

/**
 * 初始化内核页表
 * @param pg_cnt 内核物理页数
 */
static void build_k_pgtable(uint32_t pg_cnt){

	//前4M已经用了
	uint32_t paddr = (1 << 22) | 0x7;
	pg_cnt -= 1024;

	uint32_t vaddr = 0xc0400000;
	uint32_t *pte_ptr = PTE_PTR(vaddr);

	//这里页表是连续的
	while(pg_cnt--){
		*pte_ptr = paddr;
		paddr += PG_SIZE;
		++pte_ptr;
	}
	//printk("build done\n");
}

/**
 * 只被调用一次，初始化时将剩余页加入链表
 */
static void add_km_pg_cache(uint32_t s_paddr, uint32_t pgs){
	kmm.pg_cahces_cnt = pgs;
	struct page_desc *pg = paddr_to_pgdesc(s_paddr);
	while(pgs--){
		list_add(&pg->cache_tag, &kmm.page_caches);
		++pg;
	}
}

void block_desc_init(struct mem_block_desc *blk_desc){
	int i;
	int block_size = 16;
	for(i = 0; i < DESC_CNT; ++i){
		blk_desc[i].block_size = block_size;
		blk_desc[i].blocks = (PG_SIZE - sizeof(struct meta)) / block_size;
		LIST_HEAD_INIT(blk_desc[i].free_list);
		block_size *= 2;
	}
}

/**
 * 初始化内核管理区
 * @param k_pgs 内核物理页数
 * @param all_pgs 总物理页数
 * @param paddr_start 内核起始物理地址，为0
 */
static void km_manager_init(uint32_t k_pgs, uint32_t all_pgs, uint32_t paddr_start){

	ASSERT((paddr_start & ~(1 << 22)) == 0)

	uint32_t pgt_pgs = DIV_ROUND_UP(all_pgs * sizeof(struct page_desc), PG_SIZE);
	uint32_t used_pgs = 768 + pgt_pgs;

	uint32_t free_paddr_s = paddr_start + used_pgs * PG_SIZE;
	//4M对齐
	uint32_t buddy_paddr_start = (free_paddr_s + 0x003fffff) & 0xffc00000;
	//中间零散的页
	uint32_t res_pgs = (buddy_paddr_start - free_paddr_s) / PG_SIZE;
	uint32_t buddy_pgs = k_pgs - used_pgs - res_pgs;

	printk("free_paddr_s : %x\n", free_paddr_s);
	printk("res pages : %d\n", res_pgs);
	printk("buddy system paddr start : %x\n", buddy_paddr_start);
	printk("buddy system pages : %d\n", buddy_pgs);
	
	kmm.vaddr_start = K_VADDR_START;
	kmm.paddr_start = paddr_start;
	kmm.pages = k_pgs;
	kmm.buddy_paddr_start = buddy_paddr_start;
	kmm.page_table = (struct page_desc *)0xc0300000;
	kmm.v_area_saddr = K_VADDR_START + k_pgs * PG_SIZE;
		
	LIST_HEAD_INIT(kmm.v_area_head);
	LIST_HEAD_INIT(kmm.page_caches);

	printk("v_area_saddr : %x\n", kmm.v_area_saddr);

	//建立内核页表
	build_k_pgtable(k_pgs);

	//初始化页描述符
	struct page_desc *pg = kmm.page_table;
	uint32_t i = 0;
	for(; i < all_pgs; ++i){
	#ifdef DEBUG
		pg->pos = i;
	#endif
		pg->count = 0;
		++pg;
	}
	//初始化伙伴系统
	buddy_sys_init(&kmm.buddy, buddy_paddr_start, buddy_pgs);
	
	//剩余零散页加到用户内存池里
	add_km_pg_cache(free_paddr_s, res_pgs);
	//初始化块描述符
	block_desc_init(kmm.block_desc);
	mutex_lock_init(&kmm.lock);

}

/**
 * 用户内存初始化
 */
static void um_manager_init(uint32_t u_pgs, uint32_t paddr_start){
	
	umm.page_table = kmm.page_table + kmm.pages;
	umm.pages = u_pgs;
	umm.paddr_start = paddr_start;
	umm.free_pages = u_pgs;
	LIST_HEAD_INIT(umm.page_caches);
	mutex_lock_init(&umm.lock);
}	

static void mem_pool_init(uint32_t all_mem){
	
	printk("memory pool init start\n");
	uint32_t all_pgs = all_mem >> 12;
	//内核物理页，占总内存1/4
	//对1024取整
	uint32_t k_pgs = ((all_pgs >> 2) + 0x3ff) & 0xfffffc00;
	uint32_t u_pgs = all_pgs - k_pgs;
	
	printk("total pages : %d\n", all_pgs);
	printk("kernel pages : %d\n", k_pgs);
	printk("user pages : %d\n", u_pgs);

	km_manager_init(k_pgs, all_pgs, 0);
	um_manager_init(u_pgs, 0 + k_pgs * PG_SIZE);
	printk("memory pool init done\n");
}

void mem_init(){

	printk("mem_init start\n");
	//设置最大内存
	max_mm = get_mem_upper() + 0x100000;

	printk("all_memory %d\n", max_mm);
	mem_pool_init(max_mm);
	printk("mem_init done\n");
}

