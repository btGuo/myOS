#include "memory.h"
#include "stdint.h"
#include "print.h"
#include "string.h"
#include "thread.h"
#include "interrupt.h"

struct virtual_addr kernel_vaddr;
struct pool kernel_pool, user_pool;
extern struct task_struct *curr;

//元数据块，表示两种内存，以页为单位分配时，desc为null，large为true;
struct meta{
	struct mem_block_desc *desc;
	uint32_t cnt;
	bool large;
};
//内核内存块描述符
struct mem_block_desc k_block_desc[DESC_CNT];

static void mem_pool_init(uint32_t all_mem){
	put_str("memory pool init start\n");
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

	kernel_vaddr.vaddr_bitmap.byte_len = kbm_len;
	kernel_vaddr.vaddr_bitmap.bits = (void*)(MEM_BITMAP_BASE + kbm_len + ubm_len);
	kernel_vaddr.vaddr_start = VADDR_START;
	bitmap_init(&kernel_vaddr.vaddr_bitmap);

	mutex_lock_init(&kernel_pool.lock);
	mutex_lock_init(&user_pool.lock);

	put_str("memory pool init done\n");
}
//分配多页虚拟内存
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
	int vaddr_start = 0, bit_idx_start = -1;
	if(pf == PF_KERNEL){
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1)
			return NULL;
		bitmap_set_range(&kernel_vaddr.vaddr_bitmap, bit_idx_start, 1, pg_cnt);
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	}else{
		bit_idx_start = bitmap_scan(&curr->userprog_vaddr.vaddr_bitmap, \
			   	pg_cnt);
		if(bit_idx_start == -1)
			return NULL;
		bitmap_set_range(&curr->userprog_vaddr.vaddr_bitmap, \
			   	bit_idx_start, 1, pg_cnt);
		vaddr_start = curr->userprog_vaddr.vaddr_start + \
					  bit_idx_start * PG_SIZE;
	}
	return (void*)vaddr_start;
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


inline uint32_t *pte_ptr(uint32_t vaddr){
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + \
			((vaddr & 0x003ff000) >> 10));
}

inline uint32_t *pde_ptr(uint32_t vaddr){
	return (uint32_t*)(PAGE_DIR_TABLE_POS + ((vaddr & 0xffc00000) >> 20));
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
	*pte &= (~ 1);
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

void mfree_page(enum pool_flags pf, void *_vaddr, uint32_t pg_cnt){
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
	int32_t bit_idx = -1;
	if(curr->pg_dir != NULL && pf == PF_USER){

		bit_idx = (vaddr - curr->userprog_vaddr.vaddr_start) / PG_SIZE;
		bitmap_set(&curr->userprog_vaddr.vaddr_bitmap, bit_idx, 1);

	}else if(curr->pg_dir == NULL && pf == PF_KERNEL){

		bit_idx = (vaddr - kernel_vaddr.vaddr_start) / PG_SIZE;
		bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx, 1);

	}else{
		PANIC("get_a_page: error");
	}

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

void *sys_malloc(uint32_t size){
	enum pool_flags pf;
	uint32_t pool_size = 0;
	struct mem_block_desc *blk_desc = NULL;
	mem_block *blk;
	struct pool * mem_pool;

	if(curr->pg_dir){
		blk_desc = curr->u_block_desc;
		pool_size = user_pool.pool_size;
		mem_pool  = &user_pool;
		pf = PF_USER;
	}else{
		blk_desc = k_block_desc;
		pool_size = kernel_pool.pool_size;
		mem_pool = &kernel_pool;
		pf = PF_KERNEL;
	}
	if(!(size <= 0 && size > pool_size)){
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
			a->large = true;
			mutex_lock_release(&mem_pool->lock);
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
			a->large = false;
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

void sys_free(void *ptr){
	ASSERT(ptr != NULL);
	enum pool_flags pf;
	struct pool *mem_pool;
	if(curr->pg_dir){
		mem_pool = &user_pool;
		pf = PF_USER;
	}else {
		mem_pool = &kernel_pool;
		pf = PF_KERNEL;
	}

	mutex_lock_acquire(&mem_pool->lock);
	mem_block *blk = ptr;
	struct meta *m = block2meta(blk);
	if(m->desc == NULL && m->large){
		mfree_page(pf, m, m->cnt);
	}else {
		list_add_tail(blk, &m->desc->free_list);
	}
	
	mutex_lock_release(&mem_pool->lock);
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

