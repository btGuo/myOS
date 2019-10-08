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
 * 分配物理内存
 * @param pg_cnt 物理页数
 * @param pf 内存池标志，内核或者用户
 *
 * @return 分配的物理地址
 * 	@retval 0 分配失败
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

			if(pg_desc == NULL){
				PANIC("palloc no more kernel space\n");
			}
		}
		++pg_desc->count;
		return pgdesc_to_paddr(pg_desc);
	}
	
	//用户内存每次只分配一页
	ASSERT(pg_cnt == 1);
	if(list_empty(&umm.page_caches)){

		if(umm.free_pages == 0){
			PANIC("palloc no more user space\n");
		}

		pg_desc = umm.page_table + (umm.pages - umm.free_pages);
		--umm.free_pages;
	}else {
		struct list_head *lh = umm.page_caches.next;
		list_del(lh);
		pg_desc = list_entry(struct page_desc, cache_tag, lh);
	}

	//增加引用计数
	++pg_desc->count;
	return pgdesc_to_paddr(pg_desc);
}

/**
 * 释放物理内存
 */
void pfree(uint32_t paddr){

#ifdef DEBUG
	printk("pfree %x\n", paddr);
#endif
	paddr &= 0xfffff000;

	ASSERT(paddr_legal(paddr));

	struct page_desc *pg_desc = paddr_to_pgdesc(paddr);
	//printk("pg_desc->count %x\n", pg_desc->count);
	ASSERT(pg_desc->count == 1);


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
 * 如果虚拟地址对应页存在，则释放
 */
bool pg_try_free(uint32_t vaddr){
	uint32_t *pde = PDE_PTR(vaddr);
	uint32_t *pte = PTE_PTR(vaddr);
	if((*pde & 0x1) && (*pte & 0x1)){

		//printk("pg_try_free vaddr %x\n", vaddr);
		uint32_t paddr = *pte;
		ASSERT(paddr_legal(paddr));
		//检查是否共享页
		struct page_desc *pg_desc = paddr_to_pgdesc(paddr);
		if(pg_desc->count > 1){
			--pg_desc->count;
		}else {
			//printk("in pfree count %x, %x\n", pg_desc->count, paddr);
			pfree(paddr);
		}

		//删除映射，对于exit来说是不必要的
		page_table_pte_remove(vaddr);
		return true;
	}
	return false;
}

/**
 * 在动态内存区中寻找空闲页表项
 */
void *vaddr_get(uint32_t pg_cnt){

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
static inline void flush_tlb_all(){

	uint32_t paddr = addr_v2p((uint32_t)curr->pg_dir);
	//printk("flush_tlb_all paddr %x\n", paddr);
	asm volatile("movl %0, %%cr3"::"r"(paddr):"memory");
}

/**
 * 刷新指定tlb项
 *
 * @attention 这个目前好像有问题，在bochs上测试时并没有更新
 * 下面都暂且用了flush_tlb_all全部刷新
 */
static inline void flush_tlb_page(uint32_t vaddr){

	asm volatile("invlpg %0" : : "m"(vaddr) : "memory");
}

/**
 * 删除页表项
 */
void page_table_pte_remove(uint32_t vaddr){
	uint32_t *pte = PTE_PTR(vaddr);
	//p位置0
	*pte &= (~ 1);
	//刷新TLB
	//flush_tlb_page(vaddr);
	flush_tlb_all();
}

/**
 * 映射一页物理内存和虚拟内存
 * @param vaddr 虚拟地址
 * @param paddr 物理地址
 *
 * @note 注意两个参数位置，不要写反了
 */
void page_table_add(uint32_t vaddr, uint32_t paddr){
	uint32_t *pde = PDE_PTR(vaddr);
	uint32_t *pte = PTE_PTR(vaddr);
//	enum pool_flags pf = (vaddr >= 0xc0000000) ? PF_KERNEL : PF_USER;
#ifdef DEBUG
	printk("pde %x\n", (uint32_t)pde);
#endif
	ASSERT(paddr_legal(paddr));

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
		//这里必须清空
		memset((void*)((int)pte & 0xfffff000), 0, PG_SIZE);
		*pte = (paddr | 0x7);
	}
}

/**
 * 重新映射虚拟地址
 */
void page_table_remap(uint32_t vaddr, uint32_t paddr){
	uint32_t *pte = PTE_PTR(vaddr);

	ASSERT(*pte & 0x1);
	ASSERT(paddr_legal(paddr));
	*pte = (paddr | 0x7);
	//这里得刷新
	//flush_tlb_page(vaddr);
	flush_tlb_all();
}

/**
 * 分配动态内存
 */
struct v_area *v_area_get(uint32_t pg_cnt){
	uint32_t saddr = (uint32_t)vaddr_get(pg_cnt);
	if(saddr == 0)
		return NULL;

	struct v_area *n_area = kmalloc(sizeof(struct v_area));

	if(n_area == NULL){
		return NULL;
	}

	list_add_tail(&n_area->list_tag, &kmm.v_area_head);
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
	//O(n)
	while(walk != head){
		va = list_entry(struct v_area, list_tag, walk);
		if(va->s_addr == (uint32_t)vaddr){
			list_del(&va->list_tag);
			uint32_t ret = va->pg_cnt;
			//记得释放内存
			kfree(va);
			return ret;
		}
		walk = walk->next;
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
	printk("vaddr :%x\n", vaddr);
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

#ifdef DEBUG
	printk("vfree %d pages at %x\n", cnt, vaddr);
#endif

	uint32_t paddr = 0;
	//删除页表映射，释放物理页
	while(cnt--){
		paddr = addr_v2p(vaddr);
		pfree(paddr);
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
	
	uint32_t paddr = addr_v2p((uint32_t)_vaddr);
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

static inline mem_block *meta2block(struct meta *a, uint32_t idx){
	return (mem_block*)(\
			(uint32_t)a + sizeof(struct meta) + idx * a->desc->block_size);
}

static inline struct meta *block2meta(mem_block *blk){
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

			//printk("hit addr %x\n", addr);
			//TODO 确认页是否脏
			paddr = palloc(PF_USER, 1);
			paddr |= 0x7;
			*pde = paddr;
			//临时映射
			if(!vaddr){
				vaddr = vaddr_get(1);
				//printk("vaddr %x\n", vaddr);
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
	//printk("legal vaddr\n");
	vaddr &= 0xfffff000;
	
	uint32_t paddr = palloc(PF_USER, 1);
	page_table_add(vaddr, paddr);
	flush_tlb_all();

	return;
}

/**
 * 写时复制
 * TODO debug
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
