#ifndef __LIB_KERNEL_HASHTABLE_H
#define __LIB_KERNEL_HASHTABLE_H

#include "stdint.h"
#include "list.h"  //不加的话会有警告

typedef bool (*hash_func) (struct list_head *, void *);
typedef uint32_t (*hashf_func) (void *, uint32_t);

/**
 * @brief 哈希表，开哈希，数据域存放链表头
 */
struct hash_table{
	uint32_t size;   ///< 哈希表槽个数
	struct list_head *data;  ///< 数据指针
	hash_func compare;   ///< 关键字比较函数
	hashf_func hashf;   ///< 关键字映射函数
};


uint32_t hashf_uint(void *key, uint32_t max_size);
uint32_t hashf_str(void *key, uint32_t max_size);

inline void hash_table_init(struct hash_table *ht, hash_func comp, hashf_func hf);
inline void hash_table_insert(struct hash_table *ht, struct list_head *elem, void *key);
inline struct list_head *hash_table_find(struct hash_table *ht, void *key);
inline void hash_table_clear(struct hash_table *ht);
inline void hash_table_del(struct hash_table *ht);
#endif
