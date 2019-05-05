#include"memory.h"
#include"stdint.h"
#include"print.h"

struct virtual_addr kernel_vaddr;
struct pool kernel_pool, user_pool;

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

	put_str("memory pool init done\n");
}
//分配多页虚拟内存
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt){
	int vaddr_start = 0, bit_idx_start = -1;
	uint32_t cnt = 0;
	if(pf == PF_KERNEL){
		bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
		if(bit_idx_start == -1)
			return NULL;
		bitmap_set_range(&kernel_vaddr.vaddr_bitmap, bit_idx_start, 1, pg_cnt);
		vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
	}else{
	}
	return (void*)vaddr_start;
}
//分配一页物理内存
static void* palloc(struct pool *m_pool){
//	put_str("palloc start\n");
	int idx = bitmap_scan(&m_pool->pool_bitmap, 1);
	if(idx == -1){
		return NULL;
	}
	bitmap_set(&m_pool->pool_bitmap, idx, 1);
	uint32_t phy_addr = idx * PG_SIZE + m_pool->phy_addr_start;
//	put_str("palloc end\n");
	return (void*)phy_addr;
}

uint32_t *pte_ptr(uint32_t vaddr){
	return (uint32_t*)(0xffc00000 + ((vaddr & 0xffc00000) >> 10) + ((vaddr & 0x003ff000) >> 10));
}

uint32_t *pde_ptr(uint32_t vaddr){
	return (uint32_t*)(PAGE_DIR_TABLE_POS + ((vaddr & 0xffc00000) >> 20));
}
//映射一页物理内存和虚拟内存
static void page_table_add(void *_vaddr, void *_paddr){
//	put_str("page_table_add start\n");
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
//	put_str("page_table_add end\n");
}
//分配多页内存,顶层接口
void *malloc_page(enum pool_flags pf, uint32_t pg_cnt){
//	put_str("malloc_page start\n");
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
		//put_str("paddr is:");put_int((uint32_t)paddr);put_char('\n');
		if(paddr == NULL)
			return NULL;
		page_table_add((void*)vaddr, paddr);
		vaddr += PG_SIZE;
	}
//	put_str("malloc_page end\n");
	return vaddr_start;
}

void *get_kernel_pages(uint32_t pg_cnt){
	void *vaddr = malloc_page(PF_KERNEL, pg_cnt);
	if(vaddr != NULL)
		memset(vaddr, 0 , pg_cnt * PG_SIZE);
	return vaddr;
}

void mem_init(){
	put_str("mem_init start\n");
	uint32_t all_mem = *((uint32_t*)0xb03);
	mem_pool_init(all_mem);
	put_str("mem_init done\n");
}

