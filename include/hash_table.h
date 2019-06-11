#ifndef __LIB_KERNEL_HASHTABLE_H
#define __LIB_KERNEL_HASHTABLE_H

#include "stdint.h"
#include "list.h"  //不加的话会有警告

typedef bool (*hash_func) (struct list_head *, uint32_t);

/**
 * @brief 哈希表，开哈希，数据域存放链表头
 */
struct hash_table{
	uint32_t size;   ///< 哈希表槽个数
	struct list_head *data;  ///< 数据指针
	hash_func compare;
};

inline void hash_table_init(struct hash_table *ht, hash_func comp);
inline void hash_table_insert(struct hash_table *ht, struct list_head *elem, uint32_t key);
inline struct list_head *hash_table_find(struct hash_table *ht, uint32_t key);
inline void hash_table_clear(struct hash_table *ht);

#endif
