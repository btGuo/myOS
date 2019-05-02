#include"memory.h"
#include"stdint.h"
#include"print.h"

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

	kernel_pool.phy_addr_start = used_mem;
	user_pool.phy_addr_start = used_mem + kernel_free_pages * PG_SIZE;

	kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
	user_pool.pool_size = user_free_pages * PG_SIZE;

	kernel_pool.pool_bitmap.byte_len = kernel_free_pages / 8;
	user_pool.pool_bitmap.byte_len = user_free_pages / 8;

	kernel_pool.pool_bitmap.bits = (void*)MEM_BITMAP_BASE;
	user_pool.pool_bitmap.bits = (void*)(MEM_BITMAP_BASE + kernel_free_pages / 8);

	put_str("kernel_pool_phy_addr_start: ");
	put_int(kernel_pool.phy_addr_start);
	put_str("\n");

	put_str("user_pool_phy_addr_start: ");
	put_int(user_pool.phy_addr_start);
	put_str("\n");
	
	bitmap_init(&kernel_pool.pool_bitmap);
	bitmap_init(&user_pool.pool_bitmap);

	put_str("memory pool init done\n");
}
static void vaddr_get()

void mem_init(){
	put_str("mem_init start\n");
	uint32_t all_mem = *((uint32_t*)0xb03);
	mem_pool_init(all_mem);
	put_str("mem_init done\n");
}

