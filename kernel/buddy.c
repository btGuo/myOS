#include "buddy.h"
#include "memory.h"

extern struct page_desc *page_table;

uint32_t buddy_add(struct buddy_sys *buddy, uint32_t s_addr, uint32_t pg_cnt){

	ASSERT(pg_cnt <= 1024);
	struct free_area *f_area = buddy->free_area;
	struct page_desc *pg_desc = NULL;
	uint32_t pg_size = PG_SIZE;
	uint32_t i = 0;
	uint32_t paddr = s_addr;

	//修改页描述符，同时加进自由链表
	while(pg_cnt){
		if(pg_cnt & 1){
			++f_area->nr_free;
			pg_desc = paddr_to_pgdesc(paddr);
			pg_desc->order = i;
			pg_desc->free = true;
			list_add(&pg_desc->cache_tag, &f_area->free_list);
		}
		pg_cnt >>= 1;
		++f_area;
		paddr += pg_size;
		pg_size <<= 1;
	}
}

/**
 * 初始化伙伴系统
 * @param s_addr 起始地址
 * @param max_pgs 物理页数
 */
void init_buddy_sys(struct buddy_sys *buddy, uint32_t s_paddr, uint32_t pgs){

	ASSERT((s_paddr & 0x3fffff) == 0);
	ASSERT((pgs & 0x3ff) == 0);
	

	struct free_area *f_area = buddy_free + MAX_ORDER; 
	struct page_desc *pg_desc = NULL;
	uint32_t paddr = s_paddr;
	uint32_t cnt = pgs >> 10；	
	while(cnt--){
		pg_desc = paddr_to_pgdesc(paddr);
		pg_desc->order = MAX_ORDER;
		pg_desc->free = true;
		list_add(&pg_desc->cache_tag, &f_area->free_list);
		paddr += (1 << 22);
		++f_area->nr_free;
	}
}

struct page_desc *get_buddy(struct buddy_sys *buddy, struct page_desc *pg_desc){
	
	if(pg_desc->order == MAX_ORDER)
		return NULL;

	uint32_t paddr = pgdesc_to_paddr(pg_desc);
	uint32_t buddy_paddr = paddr ^ (pg_desc->order);
	return paddr_to_pgdesc(buddy_paddr);
}

struct page_desc *buddy_alloc(struct buddy_sys *buddy, uint32_t order){

	ASSERT(order <= MAX_ORDER);

	struct free_area *f_area = buddy->free_area + order;	
	uint32_t l_order = order;
	while(l_order <= MAX_ORDER){
		if(f_area->nr_free > 0){
			break;
		}
		++f_area;
		++l_order;
	}
	--f_area->nr_free;
	struct list_head *lh = f_area->free_list.next;
	list_del(lh);
	struct page_desc *pg_desc = list_entry(struct page_desc, cache_tag, lh);
	pg_desc->order = order;
	pg_desc->free = false;
	
	uint32_t paddr = pgdesc_to_paddr(pg_desc) + (1 << order) * PG_SIZE;
	buddy_add(buddy, paddr, (1 << l_order) - (1 << order));
	return pg_desc;
}

struct page_desc *merge_page(struct page_desc *pg_desc, struct page_desc *buddy_pg){

	if(pg_desc->order == MAX_ORDER)
		return NULL;
	ASSERT(pg_desc->order == buddy_pg->order);

	struct page_desc *tmp = (uint32_t)pg_desc > (uint32_t)buddy_pg ?
		pg_desc : buddy_pg;
	uint32_t paddr = pgdesc_to_paddr(pg_desc);
	struct page_desc *new_pg = paddr_to_pgdesc(paddr);
	new_pg->order = pg_desc->order + 1;
	new_pg->free = true;
}

void buddy_free(struct buddy_sys *buddy, struct page_desc *pg_desc){

	ASSERT(pg_desc->free == false);
	struct page_desc *buddy_pg = get_buddy(pg_desc);
	while(buddy_pg){

		pg_desc = merge_page(pg_desc, buddy_pg);
		buddy_pg = get_buddy(pg_desc);
	}
}

