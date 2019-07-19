#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "string.h"
#include "thread.h"
#include "interrupt.h"

#define VADDR_OFF (0xc0000000 - 0x00100000)
/**
 * 内核物理内存固定映射，由伙伴系统统一管理，用户物理内存动态映射，自由链表管理
 */

struct kmem_manager kmm;
struct umem_manager umm;

static void mem_pool_init(uint32_t all_mem){
	
	put_str("memory pool init start\n");
	uint32_t all_pgs = all_mem >> 12;
	//内核物理页，占总内存1/4
	uint32_t k_pgs = all_pgs >> 2;
	uint32_t u_pgs = all_pgs - k_pgs;
	km_manager_init(k_pgs, all_pgs, 0);
	um_manager_init(u_pgs, 0 + k_pgs * PG_SIZE);
}

uint32_t pgdesc_to_paddr(struct page_desc *pg_desc){
	return (pg_desc - kmm.page_table) * PG_SIZE;
}

struct page_desc *paddr_to_pgdesc(uint32_t paddr){
	return &kmm.page_table[(paddr >> 12)];
}

inline uint32_t *PTE_PTR(uint32_t vaddr){
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + \
			((vaddr & 0x003ff000) >> 10));
}

inline uint32_t *PDE_PTR(uint32_t vaddr){
	return (uint32_t*)(PAGE_DIR_TABLE_POS + ((vaddr & 0xffc00000) >> 20));
}

static km_manager_init(uint32_t k_pgs, uint32_t all_pgs, uint32_t paddr_start){

	ASSERT((paddr_start & ~(1 >> 22)) == 0)

	uint32_t pgt_pgs = DIV_ROUND_UP(all_pgs * sizeof(struct page_desc), PG_SIZE);
	uint32_t used_pgs = 512 + pgt_pgs;

	uint32_t free_paddr_s = paddr_start + used_pgs * PG_SIZE;
	//4M对齐
	uint32_t buddy_paddr_start = DIV_ROUND_UP(free_paddr_s, (1 << 22));
	//中间零散的页
	uint32_t res_pgs = (buddy_paddr_start - free_paddr_s) / PG_SIZE;
	
	kmm.vaddr_start = K_VADDR_START;
	kmm.paddr_start = paddr_start;
	kmm.pages = k_pgs;
	kmm.page_table = (struct page_desc *)0xc0200000;
	//建立内核页表
	build_k_pgtable(k_pgs);
	//初始化伙伴系统
	buddy_sys_init(&kmm.buddy, buddy_paddr_start, buddy_pgs);
	
	//剩余零散页加到用户内存池里
	add_km_pg_cache(free_paddr_s, res_pgs);

	block_desc_init(kmm.block_desc);
	mutex_lock_init(&kmm.lock);
}

static void add_km_pg_cache(uint32_t s_paddr, uint32_t pgs){
	struct page_desc *pg = paddr_to_pgdesc(s_paddr);
	while(pgs--){
		list_add(&pg->cache_tag, &kmm.page_caches);
		++pg;
	}
	kmm.pg_cahces_cnt = pgs;
}

static void um_manager_init(uint32_t u_pgs, uint32_t paddr_start){
	
	umm.page_table = kmm.page_table + kmm.pages;
	umm.pages = u_pgs;
	umm.paddr_start = paddr_start;
	umm.free_pages = u_pgs;
}	

uint32_t palloc(enum pool_flags pf, uint32_t pg_cnt){

	struct page_desc *pg_desc = NULL; 
	if(pf == PF_KERNEL){

		if(pg_cnt == 1 && !list_empty(&kmm.page_caches)){

			struct list_head *lh = kmm.page_caches.next;
			list_del(lh);
			pg_desc = list_entry(struct page_desc, cache_tag, lh);
		}else {
			uint32_t cnt = 1;
			uint32_t order = 0;
			while(cnt < pg_cnt){
				cnt <<= 1;
				++order;
			}
			pg_desc = buddy_alloc(&kmm.buddy, order);
		}
		return pgdesc_to_paddr(pg_desc);
	}
	
	ASSERT(pg_cnt == 1);
	if(list_empty(&umm.page_caches)){
		pg = umm.page_table + (umm.pages - umm.free_pages);
		--umm.free_pages;
	}
	struct list_head *lh = umm.page_caches.next;
	list_del(lh);
	pg_desc = list_entry(struct page_desc, cache_tag, lh);

	return pgdesc_to_paddr(pg_desc);
}

void pfree(uint32_t paddr){

	struct page_desc *pg_desc = paddr_to_pgdesc(paddr);
	if(paddr < umm.paddr_start){

		if(paddr >= kmm.buddy_paddr_start)
			buddy_free(&kmm.buddy,pg_desc);
		else 
			list_add(&pg_desc->cache_tag, &kmm.page_caches);
		return;
	}
	list_add(&pg_desc->cache_tag, &umm.page_caches);
}

void *malloc_page(enum pool_flags pf, uint32_t pg_cnt){
	
	if(pf == PF_KERNEL){
		uint32_t paddr = palloc(pf, pg_cnt);
		return (void *)(paddr + VADDR_OFF);
	}
	//TODO pf == PF_USER
}	

void free_page(enum pool_flags pf, void *_vaddr, uint32_t pg_cnt){
	
	if(pf == PF_KERNEL){
		uint32_t paddr = (uint32_t)_vaddr - VADDR_OFF;
		pfree(paddr);
	}
	//TODO
}

//申请内核内存
void *get_kernel_pages(uint32_t pg_cnt){
	mutex_lock_acquire(&kmm.lock);
	void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr != NULL)
		memset(vaddr, 0 , pg_cnt * PG_SIZE);
	mutex_lock_release(&kmm.lock);
	return vaddr;
}

//申请用户内存
//此处要上锁
void *get_user_pages(uint32_t pg_cnt){
	//TODO
}

uint32_t addr_v2p(uint32_t vaddr){
	uint32_t *pte = PTE_PTR(vaddr);
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
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

static mem_block *meta2block(struct meta *a, uint32_t idx){
	return (mem_block*)(\
			(uint32_t)a + sizeof(struct meta) + idx * a->desc->block_size);
}

static struct meta *block2meta(mem_block *blk){
	return (struct meta *)((uint32_t)blk & 0xfffff000);
}

void *kmalloc(uint32_t size){
	
	if(size <= 0 || size > kmm.pages * PG_SIZE)
		return NULL;

	struct mem_block_desc *blk_desc = kmm.block_desc;

	mutex_lock_acquire(&kmm->lock);
	//大于1024时直接分配页
	if(size > 1024){
	
		uint32_t pg_cnt = DIV_ROUND_UP(size + sizeof(struct meta), \
				PG_SIZE);
		a = malloc_page(PF_KERNEL, pg_cnt);
		if(a == NULL){
			mutex_lock_release(&kmm.lock);
			return NULL;
		}else{
			memset(a, 0, PG_SIZE * pg_cnt);
			a->desc = NULL;
			a->cnt = pg_cnt;
			mutex_lock_release(&kmm.lock);
			return (void *)(a + 1);
		}
	}else{
		struct meta *a;
		int idx;
		//找到合适的块
		for(idx = 0; idx < DESC_CNT; ++idx){
			if(size <= blk_desc[idx].block_size){
				break;
			}
		}
		//自由链表为空，重新申请内存
		if(list_empty(&blk_desc[idx].free_list)){
			//分配一页
			a = malloc_page(PF_KERNEL, 1);
			if(a == NULL){
				mutex_lock_release(&mem_pool->lock);
				return NULL;
			}
			memset(a, 0, PG_SIZE);
			//初始化元数据块
			a->desc = blk_desc + idx;
			a->cnt = blk_desc[idx].blocks;

			enum intr_status old_stat = intr_disable();
			uint32_t i;
			//加入自由链表
			for(i = 0; i < a->cnt; ++i){
				blk = meta2block(a, i);
				list_add(blk, &blk_desc[idx].free_list);
			}

			intr_set_status(old_stat);
		}
		blk = blk_desc[idx].free_list.next;
		list_del(blk);
		a = block2meta(blk);
		--a->cnt;
		mutex_lock_release(&mem_pool->lock);
		return (void *)blk;
	}
}

void kfree(void *ptr){
	
	mutex_lock_acquire(&kmm.lock);
	mem_block *blk = ptr;
	struct meta *m = block2meta(blk);
	if(m->desc == NULL){

		free_page(PF_KERNEL, m, m->cnt);
	}else {

		list_add_tail(blk, &m->desc->free_list);
		//TODO 块满时应该归还内存, 可以有别的策略
		if(++m->cnt == m->desc->blocks){
			uint32_t i = m->desc->blocks;
			mem_block *tmp = NULL;
			//删除该块在自由链表中的缓存
			while(i--){
				tmp = meta2block(m, i);
				list_del(tmp);
			}
			free_page(PF_KERNEL, m, 1);
		}
	}
		
	mutex_lock_release(&kmm.lock);
}


/**
 * 内核内存全部映射
 * @param pg_cnt 内核物理页数
 */

void build_k_pgtable(uint32_t pg_cnt){

	//前2M已经用了
	uint32_t paddr = 1 >> 21;

	pg_cnt -= 512;

	uint32_t vaddr = 0xc0100000;
	uint32_t *pde_ptr = PDE_PTR(vaddr);
	uint32_t *pte_ptr = PTE_PTR(vaddr);

	while(pg_cnt){
		uint32_t size = 1024;
		while(size-- && pg_cnt){	
			*pte_ptr = paddr;
			paddr += PG_SIZE;
			++pte_ptr;
			--pg_cnt;
		}
		++pde_ptr;
	}
}

//寻找pg_cnt 个连续虚拟页
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
	//起始虚拟地址
	uint32_t start_addr = pf == PF_KERNEL ?
		kernel_vaddr.vaddr_start :
		curr->userprog_vaddr.vaddr_start;
	//记录当前遍历到的虚拟地址
	uint32_t vaddr = start_addr;

	uint32_t *pde = PDE_PTR(start_addr);
	//最后一个页目录项不能用
	//用户虚拟内存最高为0xc0000000
	uint32_t *pde_end = (pf == PF_KERNEL) ?
		 PDE_PTR(0xffffffff) : PDE_PTR(0xbfffffff);

	uint32_t *pte = PTE_PTR(start_addr);
	uint32_t *pte_end = NULL;
	uint32_t cnt = 0;
	
	while(pde != pde_end){
		if(*pde & 0x00000001){

			pte_end = pte + 1024;
			while(pte != pte_end){
				vaddr += 4096;

				if(!(*pte & 0x00000001)){

					++cnt;
					if(cnt == pg_cnt)
						return (void *)start_addr;

				}else {
					cnt = 0;
					start_addr = vaddr;
				}
				++pte;
			}
		}else {
			//页目录为空，直接计数
			vaddr += 4096 * 1024;
			cnt += 1024;
			if(cnt >= pg_cnt)
				return (void *)start_addr;
		}
		++pde;
	}

	return NULL;
}


//映射一页物理内存和虚拟内存
static void page_table_add(void *_vaddr, void *_paddr){
	uint32_t vaddr = (uint32_t)_vaddr;
	uint32_t paddr = (uint32_t)_paddr;
	uint32_t *pde = PDE_PTR(vaddr);
	uint32_t *pte = PTE_PTR(vaddr);

	if(*pde & 0x00000001){
	//页表存在
		if(!(*pte & 0x00000001)){
			*pte = (paddr | 0x7);
		}else{
			PANIC("pte repeat");
		}
	}else{
		uint32_t pde_paddr = (uint32_t)palloc(&kernel_pool);
		*pde = (pde_paddr | 0x7);
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		*pte = (paddr | 0x7);
	}
}

static void page_table_pte_remove(uint32_t vaddr){
	uint32_t *pte = PTE_PTR(vaddr);
	//p位置0
	*pte &= (~ 1);
	//刷新TLB
	//TODO 频繁刷新会导致效率底下，应尽量一次操作完再刷新
	asm volatile("invlpg %0" : : "m"(vaddr) : "memory");
}

void copy_page_table(uint32_t *pde){

	uint32_t *pde_src = curr->pg_dir;
	uint32_t *pte_src = PTE_PTR(0);
	uint32_t cnt = 768;
	uint32_t paddr = 0;
	uint32_t *vaddr = NULL;
	while(cnt--){

		if(*pde_src & 0x1){
			//TODO 确认页是否脏
			paddr = palloc(PF_USER, 1);
			paddr |= 0x7;
			*pde = paddr;
			vaddr = vaddr_get(PF_KERNEL, 1);
			//临时映射
			page_table_add(paddr, vaddr);
			uint32_t size = 1024;
			while(size--){
				if(*pte_src & 0x1){
					*pte_src &= ~2；
					*vaddr = *pte_src;
				}
				++pte_src;
				++vaddr;
			}
			page_table_pte_remove(vaddr);
		}	
		++pde_src;
		++pde;
	}
	//复制内核页表
	cnt = 255;
	while(cnt--){
		*pde++ = *pde_src;
	}
}



			
			

