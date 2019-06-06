#include "hashtable.h"
#include "memory.h"
#include "list.h"

#define hashf(ht, x) ((x) % (ht->size))  ///< 哈希映射函数

#define quadprobe(x, i) ((x) + (i) * (i)) ///< 平方探测函数

#define HASH_SIZE (PG_SIZE * 2)     ///< 默认大小，分配两页

/**
 * @brief 初始化哈希表
 *
 * @param ht 哈希表指针
 */
inline void hash_table_init(struct hash_table *ht){
	ht->data = (struct list_head *)sys_malloc(HASH_SIZE);
	ht->length = HASH_SIZE / sizeof(struct list_head);
	uint32_t cnt = ht->length;
	while(cnt--){
		LIST_HEAD_INIT(ht->data[cnt]);
	}
}

/**
 * @brief 哈希表插入
 *
 * @param ht 哈希表指针
 * @param elem 待插入的哈希元素
 *
 */
inline void hash_table_insert(struct hash_table *ht, struct list_head *elem){
	uint32_t idx = hashf(ht, elem->key);
	list_add(elem, &ht->data[idx]);
}
/**
 * @brief 哈希表查找
 *
 * @param ht 哈希表指针
 * @param key 待查找的关键字
 *
 * @return 返回缓冲区头
 * 	@retval NULL 查找为空
 */
inline buffer_head *hash_table_find(struct hash_table *ht, uint32_t key){
	uint32_t idx = hashf(ht, key);
	struct list_head *head = ht->data[idx];
	struct list_head *cur = head->next;
	struct buffer_head *bh = NULL;

	while(cur != head){
		bh = list_entry(struct buffer_head, list_tag, cur);
		if(bh->blk_nr == key)
			return bh;
		cur = cur->next;
	}
	return NULL;
}

inline void hash_table_remove(struct hash_table *ht, \
		struct list_head *elem){
	/** empty */
}
/**
 * 清空哈希表
 */
inline void hash_table_clear(struct hash_table *ht){
	memset(data, 0, sizeof(struct struct list_head) * ht->size);
}
