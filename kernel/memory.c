#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "string.h"
#include "thread.h"
#include "interrupt.h"

struct virtual_addr kernel_vaddr;
struct pool kernel_pool, user_pool;
struct page_desc *page_table;
extern struct task_struct *curr;

//元数据块，表示两种内存，以页为单位分配时，desc为null;
struct meta{
	struct mem_block_desc *desc;
	uint32_t cnt;
};
//内核内存块描述符
struct mem_block_desc k_block_desc[DESC_CNT];

static void mem_pool_init(uint32_t all_mem){
	put_str("memory pool init start\n");
	uint32_t all_pages = all_mem / PG_SIZE;
	//页目录1 + 页表项255
	uint32_t page_table_size = PG_SIZE * 256;
	//低端1M内存加页表占用内存
	uint32_t used_mem = page_table_size + 0x100000;
	uint32_t free_mem = all_mem - used_mem;
	uint16_t free_pages = free_mem / PG_SIZE;

	uint16_t kernel_free_pages = free_pages / 4;
	uint16_t user_free_pages = free_pages - kernel_free_pages;

	uint32_t kbm_len = kernel_free_pages / 8;
	uint32_t ubm_len = user_free_pages / 8;

	kernel_pool.phy_addr_start = used_mem;
	user_pool.phy_addr_start = used_mem + kernel_free_pages * PG_SIZE;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.byte_len = kbm_len;
	user_pool.pool_bitmap.byte_len = ubm_len;

	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_len);

	put_str("kernel_pool_phy_addr_start: ");
	put_int(kernel_pool.phy_addr_start);
	put_str("\n");

	put_str("user_pool_phy_addr_start: ");
	put_int(user_pool.phy_addr_start);
	put_str("\n");
	
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);

//	kernel_vaddr.vaddr_bitmap.byte_len = kbm_len;
//	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_len + ubm_len);
	kernel_vaddr.vaddr_start = VADDR_START;
//	bitmap_init(&kernel_vaddr.vaddr_bitmap);

	mutex_lock_init(&kernel_pool.lock);
	mutex_lock_init(&user_pool.lock);

	uint32_t pg_nums = sizeof(struct page_desc) * all_mem / PG_SIZE;
	page_table = malloc_page(PF_KERNEL, pg_nums);
	if(!page_table){
		PANIC("we need more space!\n");
	}
	put_str("memory pool init done\n");
}

inline uint32_t *pte_ptr(uint32_t vaddr){
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + \
			((vaddr & 0x003ff000) >> 10));
}

inline uint32_t *pde_ptr(uint32_t vaddr){
	return (uint32_t*)(PAGE_DIR_TABLE_POS + ((vaddr & 0xffc00000) >> 20));
}

//寻找pg_cnt 个连续虚拟页
static void *vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
	//起始虚拟地址
	uint32_t start_addr = pf == PF_KERNEL ?
		kernel_vaddr.vaddr_start :
		curr->userprog_vaddr.vaddr_start;
	//记录当前遍历到的虚拟地址
	uint32_t vaddr = start_addr;

	uint32_t *pde = pde_ptr(start_addr);
	//最后一个页目录项不能用
	//用户虚拟内存最高为0xc0000000
	uint32_t *pde_end = (pf == PF_KERNEL) ?
		 pde_ptr(0xffffffff) : pde_ptr(0xbfffffff);

	uint32_t *pte = pte_ptr(start_addr);
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


//分配一页物理内存
static void* palloc(struct pool *m_pool){
	int idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if(idx == -1){
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, idx, 1);
	uint32_t phy_addr = idx * PG_SIZE + m_pool->phy_addr_start;
	return (void*)phy_addr;
}

//释放一页物理页
void pfree(uint32_t paddr){
	struct pool *mem_pool;
	uint32_t idx = 0;
	if(paddr >= user_pool.phy_addr_start){
		mem_pool = &user_pool;
	}else {
		mem_pool = &kernel_pool;
	}
	idx = paddr - mem_pool->phy_addr_start;
	bitmap_set(&mem_pool->pool_bitmap, idx, 0);
}


//映射一页物理内存和虚拟内存
static void page_table_add(void *_vaddr, void *_paddr){
	uint32_t vaddr = (uint32_t)_vaddr;
	uint32_t paddr = (uint32_t)_paddr;
	uint32_t *pde = pde_ptr(vaddr);
	uint32_t *pte = pte_ptr(vaddr);

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
	uint32_t *pte = pte_ptr(vaddr);
	//p位置0
	*pte &= (~ 1);
	//刷新TLB
	asm volatile("invlpg %0" : : "m"(vaddr) : "memory");
}

//分配多页内存,顶层接口
//先寻找物理内存和虚拟内存，映射后返回地址
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt){
	struct pool *mem_pool = NULL;
	if(pf == PF_KERNEL){
		ASSERT(pg_cnt > 0 && pg_cnt < kernel_pool.pool_size / PG_SIZE);
		mem_pool = &kernel_pool;
	}else{
		ASSERT(pg_cnt > 0 && pg_cnt < user_pool.pool_size / PG_SIZE);
		mem_pool = &user_pool;
	}

	void *vaddr_start = vaddr_get(pf, pg_cnt);
	uint32_t vaddr = (uint32_t)vaddr_start;

	if(vaddr_start == NULL)
		return NULL;
	while(pg_cnt--){
		void *paddr = palloc(mem_pool);
		if(paddr == NULL)
			return NULL;
		page_table_add((void*)vaddr, paddr);
		vaddr += PG_SIZE;
	}
	return vaddr_start;
}

//释放多页内存，顶层接口
//pf其实不需要
void free_page(enum pool_flags pf, void *_vaddr, uint32_t pg_cnt){
	uint32_t vaddr = (uint32_t)_vaddr;
	uint32_t paddr = addr_v2p(vaddr);
	while(pg_cnt--){
		pfree(paddr);
		page_table_pte_remove(vaddr);

		vaddr += PG_SIZE;
		paddr = addr_v2p(vaddr);
	}
}

//申请内核内存
void *get_kernel_pages(uint32_t pg_cnt){
	void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr != NULL)
		memset(vaddr, 0 , pg_cnt * PG_SIZE);
	return vaddr;
}

//申请用户内存
//此处要上锁
void *get_user_pages(uint32_t pg_cnt){
	mutex_lock_acquire(&user_pool.lock);
	void *vaddr = malloc_page(PF_USER, pg_cnt);
	if(vaddr != NULL)
		memset(vaddr, 0, pg_cnt * PG_SIZE);
	mutex_lock_release(&user_pool.lock);
	return vaddr;
}

//指定虚拟内存，只映射一页
void *get_a_page(enum pool_flags pf, uint32_t vaddr){
	struct pool *mem_pool = pf & PF_KERNEL ? &kernel_pool :&user_pool;
	mutex_lock_acquire(&mem_pool->lock);

	void *paddr = palloc(mem_pool);
	if(paddr == NULL)
		return NULL;
	page_table_add((void*)vaddr, paddr);
	mutex_lock_release(&mem_pool->lock);
	return (void*)vaddr;
}

uint32_t addr_v2p(uint32_t vaddr){
	uint32_t *pte = pte_ptr(vaddr);
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

static void *_malloc(enum pool_flags pf, uint32_t size){

	uint32_t pool_size = 0;
	struct mem_block_desc *blk_desc = NULL;
	mem_block *blk;
	struct pool * mem_pool;

	if(pf == PF_USER){

		blk_desc = curr->u_block_desc;
		pool_size = user_pool.pool_size;
		mem_pool  = &user_pool;

	}else if(pf == PF_KERNEL){

		blk_desc = k_block_desc;
		pool_size = kernel_pool.pool_size;
		mem_pool = &kernel_pool;
	}else {
		PANIC("pool_flags error\n");
	}

	if(size <= 0 || size > pool_size){
		return NULL;
	}

	mutex_lock_acquire(&mem_pool->lock);
	struct meta *a;
	//大于1024时直接分配页
	if(size > 1024){
		uint32_t pg_cnt = DIV_ROUND_UP(size + sizeof(struct meta), \
				PG_SIZE);
		a = malloc_page(pf, pg_cnt);
		if(a == NULL){
			mutex_lock_release(&mem_pool->lock);
			return NULL;
		}else{
			memset(a, 0, PG_SIZE * pg_cnt);
			a->desc = NULL;
			a->cnt = pg_cnt;
			mutex_lock_release(&mem_pool->lock);
			//TODO 这里分配的大小可能会有问题，减去meta后可能不够阿
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
			a = malloc_page(pf, 1);
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

static void _free(enum pool_flags pf, void *ptr){

	struct pool *mem_pool;
	if(pf == PF_USER){
		
		mem_pool = &user_pool;

	}else if(pf == PF_KERNEL){

		mem_pool = &kernel_pool;
	}else {
		PANIC("pool flag error\n");
	}

	mutex_lock_acquire(&mem_pool->lock);
	mem_block *blk = ptr;
	struct meta *m = block2meta(blk);
	if(m->desc == NULL){
		free_page(pf, m, m->cnt);
	}else {
		list_add_tail(blk, &m->desc->free_list);
		//块满时应该归还内存, 可以有别的策略
		if(++m->cnt == m->desc->blocks){
			uint32_t i = m->desc->blocks;
			mem_block *tmp = NULL;
			//删除该块在自由链表中的缓存
			while(i--){
				tmp = meta2block(m, i);
				list_del(tmp);
			}
			free_page(pf, m, 1);
		}
	}
	
	mutex_lock_release(&mem_pool->lock);
}


/**
 * 按字节分配内存
 */
void *sys_malloc(uint32_t size){

	if(curr->pg_dir){
		return _malloc(PF_USER, size);
	}
	return _malloc(PF_KERNEL, size);
}	

void sys_free(void *ptr){
	ASSERT(ptr != NULL);

	if(curr->pg_dir){

		_free(PF_USER, ptr);
	}else {
		_free(PF_KERNEL, ptr);
	}
}

/**
 * 分配内核内存
 */
void *kmalloc(uint32_t size){

	return _malloc(PF_KERNEL, size);
}

/**
 * 释放内核内存
 */
void kfree(void *ptr){
	ASSERT(ptr != NULL);
	_free(PF_KERNEL, ptr);
}

void mem_init(){
	put_str("mem_init start\n");
	mutex_lock_init(&user_pool.lock);
	mutex_lock_init(&kernel_pool.lock);
	uint32_t all_mem = *((uint32_t*)0xb00);
	mem_pool_init(all_mem);
	block_desc_init(k_block_desc);
	put_str("mem_init done\n");
}

