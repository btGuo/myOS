#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "string.h"
#include "thread.h"
#include "interrupt.h"
#include "buddy.h"
#include "sync.h"
#include "process.h"

/**
 * 内核物理内存固定映射，由伙伴系统统一管理，用户物理内存动态映射，自由链表管理
 */

struct kmem_manager kmm;
struct umem_manager umm;

inline uint32_t pgdesc_to_paddr(struct page_desc *pg_desc){
	return (pg_desc - kmm.page_table) * PG_SIZE;
}

inline struct page_desc *paddr_to_pgdesc(uint32_t paddr){
	return &kmm.page_table[(paddr >> 12)];
}

inline uint32_t *PTE_PTR(uint32_t vaddr){
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + \
			((vaddr & 0x003ff000) >> 10));
}

inline uint32_t *PDE_PTR(uint32_t vaddr){
	return (uint32_t*)(0xfffff000 + ((vaddr & 0xffc00000) >> 20));
}

/**
 * 初始化内核页表
 * @param pg_cnt 内核物理页数
 */
static void build_k_pgtable(uint32_t pg_cnt){

	//前2M已经用了
	uint32_t paddr = (1 << 21) | 0x7;
	pg_cnt -= 512;

	uint32_t vaddr = 0xc0100000;
	uint32_t *pte_ptr = PTE_PTR(vaddr);

	//这里页表是连续的
	while(pg_cnt--){
		*pte_ptr = paddr;
		paddr += PG_SIZE;
		++pte_ptr;
	}
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
	uint32_t used_pgs = 512 + pgt_pgs;

	uint32_t free_paddr_s = paddr_start + used_pgs * PG_SIZE;
	//4M对齐
	uint32_t buddy_paddr_start = (free_paddr_s + 0x003fffff) & 0xffc00000;
	//中间零散的页
	uint32_t res_pgs = (buddy_paddr_start - free_paddr_s) / PG_SIZE;
	uint32_t buddy_pgs = k_pgs - used_pgs - res_pgs;

	printk("free_paddr_s : %h\n", free_paddr_s);
	printk("res pages : %d\n", res_pgs);
	printk("buddy system paddr start : %h\n", buddy_paddr_start);
	printk("buddy system pages : %d\n", buddy_pgs);
	
	kmm.vaddr_start = K_VADDR_START;
	kmm.paddr_start = paddr_start;
	kmm.pages = k_pgs;
	kmm.buddy_paddr_start = buddy_paddr_start;
	kmm.page_table = (struct page_desc *)0xc0100000;
	kmm.v_area_saddr = K_VADDR_START + (k_pgs - 256) * PG_SIZE;
	LIST_HEAD_INIT(kmm.v_area_head);
	LIST_HEAD_INIT(kmm.page_caches);

	printk("v_area_saddr : %h\n", kmm.v_area_saddr);

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
	uint32_t k_pgs = all_pgs >> 2;
	uint32_t u_pgs = all_pgs - k_pgs;
	
	printk("total pages : %d\n", all_pgs);
	printk("kernel pages : %d\n", k_pgs);
	printk("user pages : %d\n", u_pgs);

	km_manager_init(k_pgs, all_pgs, 0);
	um_manager_init(u_pgs, 0 + k_pgs * PG_SIZE);
	printk("memory pool init done\n");
}


/**
 * 分配物理内存
 * @param pg_cnt 物理页数
 */
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
	
	//用户内存每次只分配一页
	ASSERT(pg_cnt == 1);
	if(list_empty(&umm.page_caches)){

		pg_desc = umm.page_table + (umm.pages - umm.free_pages);
		--umm.free_pages;
	}else {
		struct list_head *lh = umm.page_caches.next;
		list_del(lh);
		pg_desc = list_entry(struct page_desc, cache_tag, lh);
	}

	++pg_desc->count;
	return pgdesc_to_paddr(pg_desc);
}

/**
 * 释放物理内存
 */
void pfree(uint32_t paddr){

#ifdef DEBUG
	printk("pfree %h\n", paddr);
#endif
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

/**
 * 在动态内存区中寻找空闲页表项
 */
static void *vaddr_get(uint32_t pg_cnt){

	uint32_t saddr = kmm.v_area_saddr;
	uint32_t eaddr = 0xffc00000;
	uint32_t vaddr = saddr;
	uint32_t cnt = 0;

	uint32_t *pde = PDE_PTR(saddr);
	uint32_t *pte = PTE_PTR(saddr);
	uint32_t *pde_end = PDE_PTR(eaddr);
	uint32_t *pte_end = NULL;

	while(pde != pde_end){

		if(*pde & 0x1){
			
			pte_end = pte + 1024;
			while(pte != pte_end){

				vaddr += PG_SIZE;
				if(!(*pte & 0x1)){

					++cnt;
					if(cnt == pg_cnt)
						return (void*)saddr;
				}else {
					cnt = 0;
					saddr = vaddr;
				}
				++pte;
			}
		}else {
			vaddr += PG_SIZE * 1024;
			cnt += 1024;
			if(cnt >= pg_cnt){
				return (void*)saddr;
			}
		}
		++pde;
	}
	return NULL;
}

//TODO 检查tlb刷新问题

/**
 * 刷新全部tlb
 */
void flush_tlb_all(){

	uint32_t paddr = addr_v2p((uint32_t)curr->pg_dir);
	asm volatile("movl %0, %%cr3"::"r"(paddr):"memory");
}

/**
 * 刷新指定tlb项
 */
#define FLUSH_TLB_PAGE(vaddr) asm volatile("invlpg %0" : : "m"(vaddr) : "memory")

/**
 * 删除页表项
 */
static void page_table_pte_remove(uint32_t vaddr){
	uint32_t *pte = PTE_PTR(vaddr);
	//p位置0
	*pte &= (~ 1);
	//刷新TLB
	FLUSH_TLB_PAGE(vaddr);
}

/**
 * 映射一页物理内存和虚拟内存
 * @param vaddr 虚拟地址
 * @param paddr 物理地址
 *
 * @note 注意两个参数位置，不要写反了
 */
static void page_table_add(uint32_t vaddr, uint32_t paddr){
	uint32_t *pde = PDE_PTR(vaddr);
	uint32_t *pte = PTE_PTR(vaddr);
//	enum pool_flags pf = (vaddr >= 0xc0000000) ? PF_KERNEL : PF_USER;
#ifdef DEBUG
	printk("pde %h\n", (uint32_t)pde);
#endif

	if(*pde & 0x1){
	//页表存在
		if(!(*pte & 0x1)){
			*pte = (paddr | 0x7);
		}else{
			PANIC("pte repeat");
		}
	}else{
		//这里拿user页
		uint32_t pde_paddr = palloc(PF_USER, 1);
		*pde = (pde_paddr | 0x7);
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		*pte = (paddr | 0x7);
	}
}

static void page_table_remap(uint32_t vaddr, uint32_t paddr){
	uint32_t *pte = PTE_PTR(vaddr);

	ASSERT(*pte & 0x1);
	*pte = (paddr | 0x7);
	//这里得刷新
	FLUSH_TLB_PAGE(vaddr);
}

/**
 * 分配动态内存
 */
struct v_area *v_area_get(uint32_t pg_cnt){
	uint32_t saddr = (uint32_t)vaddr_get(pg_cnt);
	if(saddr == 0)
		return NULL;

	struct v_area *n_area = kmalloc(sizeof(struct v_area));
	list_add_tail(&kmm.v_area_head, &n_area->list_tag);
	n_area->s_addr = saddr;
	n_area->pg_cnt = pg_cnt;
	return n_area;
}

/**
 * 删除动态内存
 * @return 删除的虚拟页数
 * 	@retval 0 删除失败
 */
uint32_t v_area_del(uint32_t *vaddr){
	struct list_head *head = &kmm.v_area_head;
	struct list_head *walk = head->next;
	struct v_area *va = NULL;
	while(walk != head){
		va = list_entry(struct v_area, list_tag, walk);
		if(va->s_addr == (uint32_t)vaddr){
			list_del(&va->list_tag);
			uint32_t ret = va->pg_cnt;
			//记得释放内存
			kfree(va);
			return ret;
		}
	}
	return 0;
}	

/**
 * 分配非连续内存，以页为单位
 * @return 分配地址
 * 	@retval NULL 分配失败
 */
void *vmalloc(uint32_t pg_cnt){
	mutex_lock_acquire(&kmm.lock);
	//获取虚拟地址
	struct v_area *va = v_area_get(pg_cnt);
	if(va == NULL){
		return NULL;
	}

	//添加页表映射
	uint32_t paddr = 0;
	uint32_t vaddr = va->s_addr;
#ifdef DEBUG
	printk("vaddr :%h\n", vaddr);
#endif
	while(pg_cnt--){
		paddr = palloc(PF_USER, 1);
		page_table_add(vaddr, paddr);
		vaddr += PG_SIZE;
	}
	mutex_lock_release(&kmm.lock);
	return (void *)va->s_addr;
}

void vfree(void *_vaddr){
	
	mutex_lock_acquire(&kmm.lock);
	uint32_t cnt = v_area_del(_vaddr);
	uint32_t vaddr = (uint32_t)_vaddr;

	//删除页表映射
	while(cnt--){
		page_table_pte_remove(vaddr);
		vaddr += PG_SIZE;
	}

	mutex_lock_release(&kmm.lock);
}

/**
 * 分配内存
 */
static void *kmalloc_page(uint32_t pg_cnt){
	
	uint32_t paddr = palloc(PF_KERNEL, pg_cnt);
	return (void *)(paddr + VADDR_OFF);
}	

/**
 * 释放内存
 */
static void kfree_page(void *_vaddr){
	
	uint32_t paddr = (uint32_t)_vaddr - VADDR_OFF;
	pfree(paddr);
	return;
}

/**
 * 以页为单位申请内存
 */
void *get_kernel_pages(uint32_t pg_cnt){
	mutex_lock_acquire(&kmm.lock);
	void *vaddr = kmalloc_page(pg_cnt);
	if(vaddr != NULL)
		memset(vaddr, 0 , pg_cnt * PG_SIZE);
	mutex_lock_release(&kmm.lock);
	return vaddr;
}

void free_kernel_pages(void *vaddr){

	mutex_lock_acquire(&kmm.lock);
	kfree_page(vaddr);
	mutex_lock_release(&kmm.lock);
}

uint32_t addr_v2p(uint32_t vaddr){
	uint32_t *pte = PTE_PTR(vaddr);
	return ((*pte & 0xfffff000) + (vaddr & 0x00000fff));
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
	struct meta *a;
	mem_block *blk;

	mutex_lock_acquire(&kmm.lock);
	//大于1024时直接分配页
	if(size > 1024){
	
		uint32_t pg_cnt = DIV_ROUND_UP(size + sizeof(struct meta), \
				PG_SIZE);
		a = kmalloc_page(pg_cnt);
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
			a = kmalloc_page(1);
			if(a == NULL){
				mutex_lock_release(&kmm.lock);
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
		mutex_lock_release(&kmm.lock);
		return (void *)blk;
	}
}

void kfree(void *ptr){
	
	mutex_lock_acquire(&kmm.lock);
	mem_block *blk = ptr;
	struct meta *m = block2meta(blk);
	if(m->desc == NULL){

		kfree_page(m);
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
			kfree_page(m);
		}
	}
		
	mutex_lock_release(&kmm.lock);
}

/**
 * 指定虚拟内存，只映射一页
 */
void *get_a_page(enum pool_flags pf, uint32_t vaddr){
	if(pf == PF_KERNEL) mutex_lock_acquire(&kmm.lock);
	else mutex_lock_acquire(&umm.lock);
	
	uint32_t paddr = palloc(pf, 1);
	page_table_add(vaddr, paddr);

	if(pf == PF_KERNEL) mutex_lock_release(&kmm.lock);
	else mutex_lock_release(&umm.lock);

	return (void*)vaddr;
}

/**
 * 复制页表，fork调用
 * @param pde 目标页目录
 */
void copy_page_table(uint32_t *pde){

#ifdef DEBUG	
	printk("copy_page_table start\n");
#endif

	uint32_t *pde_src = curr->pg_dir;
	uint32_t cnt = 768;
	uint32_t addr = 0;
	uint32_t paddr = 0;
	uint32_t *vaddr = NULL;
	struct page_desc *pg_desc = NULL;
	while(cnt--){

		if(*pde_src & 0x1){
#ifdef DEBUG
			printk("hit addr %h\n", addr);
#endif
			//TODO 确认页是否脏
			paddr = palloc(PF_USER, 1);
			paddr |= 0x7;
			*pde = paddr;
			//临时映射
			if(!vaddr){
				vaddr = vaddr_get(1);
				//printk("vaddr %h\n", vaddr);
				page_table_add((uint32_t)vaddr, paddr);
			}else {
				page_table_remap((uint32_t)vaddr, paddr);
			}
			uint32_t size = 1024;
			uint32_t *pte_src = PTE_PTR(addr);
			while(size--){
				if(*pte_src & 0x1){

					//记得增加引用计数
					pg_desc = paddr_to_pgdesc(*pte_src);
					++pg_desc->count;

					//写保护
					*pte_src &= ~2;
					*vaddr = *pte_src;
				}else {
					//这里记得置0
					*vaddr = 0;
				}
				++pte_src;
				++vaddr;
			}
			//还原
			vaddr -= 1024;
		}	
		addr += (1 << 22);
		++pde_src;
		++pde;
	}
	page_table_pte_remove((uint32_t)vaddr);
#ifdef DEBUG
	printk("copy_page_table done\n");
#endif
}

void mem_init(){

	printk("mem_init start\n");
	uint32_t all_mem = *((uint32_t*)0xb00);
	mem_pool_init(all_mem);
	printk("mem_init done\n");
}

/**
 * 缺页异常处理
 */
void do_page_fault(uint32_t vaddr){

	if(vaddr >= 0xc0000000){
		PANIC("error page\n");		
		return;
	}
	
	if(!is_in_vm_area(vaddr)){
		PANIC("error page not in vm_area\n");
		return;
	}

	//地址合法
	printk("legal vaddr\n");
	vaddr &= 0xfffff000;
	
	uint32_t paddr = palloc(PF_USER, 1);
	page_table_add(vaddr, paddr);

	return;
}

/**
 * 写时复制
 */
void do_wp_page(uint32_t _vaddr){

#ifdef DEBUG
	printk("do write protect page\n");
#endif

	_vaddr &= 0xfffff000;
	uint32_t *vaddr = (uint32_t *)_vaddr;

	uint32_t paddr = addr_v2p(_vaddr);
	uint32_t *pte_ptr = PTE_PTR(_vaddr);
	struct page_desc *pg_desc = paddr_to_pgdesc(paddr);

	if(pg_desc->count == 0){
		PANIC("error\n");
		return;
	}

#ifdef DEBUG
	printk("count : %d\n", pg_desc->count);
#endif

	if(pg_desc->count == 1){
		*pte_ptr |= 2;
		return;
	}

	--pg_desc->count;

	//建立临时映射
	uint32_t *swap_vaddr = vaddr_get(1);
	page_table_add((uint32_t)swap_vaddr, paddr);

	//分配新物理地址，映射当前页
	uint32_t new_paddr = palloc(PF_USER, 1);
	page_table_remap((uint32_t)vaddr, new_paddr);

	//全部刷新
	flush_tlb_all();

	//复制当前页
	uint32_t cnt = 1024;
	uint32_t *src = swap_vaddr;
	while(cnt--){
		*vaddr++ = *src++;
	}
	//删除临时映射
	page_table_pte_remove((uint32_t)swap_vaddr);

#ifdef DEBUG
	printk("done write protect page\n");
#endif
}

void *sys_malloc(uint32_t size){
	//TODO
	return NULL;
}

void sys_free(void *ptr){
	//TODO
}
