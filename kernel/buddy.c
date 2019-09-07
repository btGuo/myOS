#include <buddy.h>
#include <memory.h>

void buddy_add(struct buddy_sys *buddy, uint32_t s_addr, uint32_t pg_cnt){

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
#ifdef DEBUG
			printk("pos %d, order %d  ", pg_desc->pos, pg_desc->order);
#endif
			list_add(&pg_desc->cache_tag, &f_area->free_list);
			paddr += pg_size;
		}
		pg_cnt >>= 1;
		++f_area;
		pg_size <<= 1;
		++i;
	}
#ifdef DEBUG
	printk("\n");
#endif
}

/**
 * 初始化伙伴系统
 * @param s_addr 起始地址
 * @param max_pgs 物理页数
 */
void buddy_sys_init(struct buddy_sys *buddy, uint32_t s_paddr, uint32_t pgs){

	printk("buddy_sys_init start\n");
	//4M对齐
	ASSERT((s_paddr & 0x3fffff) == 0);
	ASSERT((pgs & 0x3ff) == 0);

	//初始化free_area
	uint32_t cnt = MAX_ORDER + 1;
	struct free_area *f_area = buddy->free_area;
	while(cnt--){
		f_area->nr_free = 0;
		LIST_HEAD_INIT(f_area->free_list);
		++f_area;
	}
	
	f_area = buddy->free_area + MAX_ORDER; 
	struct page_desc *pg_desc = NULL;
	uint32_t paddr = s_paddr;
	cnt = pgs >> 10;
	while(cnt--){
		pg_desc = paddr_to_pgdesc(paddr);
		pg_desc->order = MAX_ORDER;
		pg_desc->free = true;
		list_add_tail(&pg_desc->cache_tag, &f_area->free_list);
		paddr += (1 << 22);
		++f_area->nr_free;
	}
	printk("buddy_sys_init done\n");
}

/**
 * 找到该页的伙伴页
 */
struct page_desc *get_buddy(struct buddy_sys *buddy, struct page_desc *pg_desc){
	
	//最大页没有伙伴
	if(pg_desc->order == MAX_ORDER)
		return NULL;

	uint32_t paddr = pgdesc_to_paddr(pg_desc);
	uint32_t buddy_paddr = paddr ^ (PG_SIZE << pg_desc->order);
	//printk("* %h %h *", paddr, buddy_paddr);
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
#ifdef DEBUG
	printk("pg_desc->pos %d\n", pg_desc->pos);
	printk("paddr %h\n", pgdesc_to_paddr(pg_desc));
#endif
	pg_desc->order = order;
	pg_desc->free = false;
	
	uint32_t paddr = pgdesc_to_paddr(pg_desc) + (1 << order) * PG_SIZE;
	buddy_add(buddy, paddr, (1 << l_order) - (1 << order));
	return pg_desc;
}

struct page_desc *merge_page(struct page_desc *pg_desc, struct page_desc *buddy_pg){

	ASSERT(pg_desc->order == buddy_pg->order);

	//小页框即是目标	
	struct page_desc *new_pg = (uint32_t)pg_desc < (uint32_t)buddy_pg ?
		pg_desc : buddy_pg;
	new_pg->order = pg_desc->order + 1;
	new_pg->free = true;
	return new_pg;
}

void buddy_free(struct buddy_sys *buddy, struct page_desc *pg_desc){

	ASSERT(pg_desc->free == false);
	struct free_area *f_area = buddy->free_area;

	pg_desc->free = true;
	uint32_t order = pg_desc->order;
	f_area[order].nr_free++;
	list_add(&pg_desc->cache_tag, &f_area[order].free_list);

	if(order == MAX_ORDER){
		return;
	}

	struct page_desc *buddy_pg = get_buddy(buddy, pg_desc);
#ifdef DEBUG
	printk("buddy_pg : %d\n", buddy_pg->pos);
#endif

	while(order < MAX_ORDER && buddy_pg->free){

		//从旧区域中删除
		f_area[order].nr_free -= 2;
		list_del(&pg_desc->cache_tag);
		list_del(&buddy_pg->cache_tag);

		++order;
		//加入新区域
		f_area[order].nr_free++;
		pg_desc = merge_page(pg_desc, buddy_pg);
#ifdef DEBUG
		printk("pos %d, order %d, ", pg_desc->pos, pg_desc->order);
#endif
		list_add(&pg_desc->cache_tag, &f_area[order].free_list);

		buddy_pg = get_buddy(buddy, pg_desc);
	}
#ifdef DEBUG
	printk("\n");
#endif
}

