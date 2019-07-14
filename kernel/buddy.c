#include "buddy.h"
#include "memory.h"

extern struct page_desc *page_table;

uint32_t buddy_add(struct buddy_sys *buddy, uint32_t s_addr, uint32_t pg_cnt){

	ASSERT(pg_cnt < 1024);
	struct free_area *f_area = buddy->free_area;
	struct page_desc *pg_desc = NULL;
	uint32_t pg_size = PG_SIZE;
	uint32_t i = 0;
	uint32_t cur_addr = s_addr;

	//修改页描述符，同时加进自由链表
	while(pg_cnt){
		if(pg_cnt & 1){
			++f_area->nr_free;
			pg_desc = page_table + (cur_addr >> 12);
			list_add(&pg_desc->cache_tag, f_area->free_list);
			pg_desc->order = i;
		}
		pg_cnt >>= 1;
		++f_area;
		cur_addr += pg_size;
		pg_size <<= 1;
	}
}

void buddy_add_max(struct buddy_sys *buddy, uint32_t s_addr, uint32_t pg_cnt){
	//大于4M的内存块加入链表，初始化时用

	struct free_area *f_area = buddy->free_area + MAX_ORDER - 1;		
	struct page_desc *pg_desc = NULL;
	
	uint32_t cur_addr = s_addr;
	uint32_t max_blk_size = PG_SIZE << (MAX_ORDER - 1);
	uint32_t i = 1;
	while(pg_cnt){
		if(pg_cnt & 1){
			uint32_t cnt = 1 << i;
			f_area->nr_free += cnt;
			while(cnt--){
				pg_desc = page_table + (cur_addr >> 12);
				cur_addr += max_blk_size;
				list_add(&pg_desc->cache_tag, f_area->free_list);
				pg_desc->order = MAX_ORDER - 1;
			}
		}else{
			cur_addr += pg_size;
		}
		pg_cnt >>= 1;
	}
}
			

/**
 * 初始化伙伴系统
 * @param s_addr 起始地址
 * @param max_pgs 物理页数
 */
void init_buddy_sys(struct buddy_sys *buddy, uint32_t s_addr, uint32_t max_pgs){
	//TODO 考虑max_pgs < 4M

	uint32_t cur_addr = s_addr;
	//4K对齐
	ASSERT((s_addr & 0xfff) == 0);
	

	//剩余的全部放到最大区域内
	
}

uint32_t *buddy_alloc(struct buddy_sys *buddy, uint32_t pg_cnt){

	ASSERT(pg_cnt <= 1024);
	uint32_t l_cnt = 1;
	uint32_t l_order = 0;
	while(l_cnt < pg_cnt){
		l_cnt <<= 1;
		++l_order;
	}
	ASSERT(l_order < MAX_ORDER);


	uint32_t order = l_order;
	struct free_area *f_area = buddy->free_area + order;	
	while(order < MAX_ORDER){
		if(f_area->nr_free > 0){
			break;
		}
		++f_area;
		++order;
	}
	--f_area->nr_free;
	struct list_head *lh = f_area->free_list->next;
	list_del(lh);
	struct page_desc *pg_desc = list_entry(struct page_desc, cache_tag, lh);

	//计算物理地址
	uint32_t paddr = buddy->start_addr + (pg_desc - page_table);
	uint32_t cur_addr = paddr + l_order * PG_SIZE;

	uint32_t res_cnt = (1 << order) - l_cnt;
	uint32_t pg_size = PG_SIZE;
	uint32_t i = 0;
	f_area = buddy->free_area;
	while(res_cnt){
		if(res_cnt & 1){
			++f_area->nr_free;
			pg_desc = page_table + (cur_addr >> 12);
			list_add(&pg_desc->cache_tag, f_area->free_list);
			pg_desc->order = i;
		}
		++i;
		cur_addr += pg_size;
		pg_size <<= 1;
		res_cnt >>= 1;	
	}
}
