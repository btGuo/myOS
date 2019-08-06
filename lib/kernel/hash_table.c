#include "hash_table.h"
#include "list.h"
#include "memory.h"
#include "string.h"

#define HASH_SIZE (PG_SIZE * 2)     ///< 默认大小，分配两页


/**
 * 整数哈希函数
 * @param key 关键字指针
 * @param max_size 哈希表大小
 */
uint32_t hashf_uint(void *key, uint32_t max_size){
	uint32_t *_key = (uint32_t *)key;
	return *_key % max_size;
}

/**
 * bkdr字符串哈希函数
 */
static uint32_t bkdr_hash(const char *str){
	uint32_t seed = 131;  //31 131 1313 etc..
	uint32_t hashv = 0;
	while(*str){
		hashv = hashv * seed + (*str++);
	}
	return (hashv & 0x7fffffff);
}

uint32_t hashf_str(void *key, uint32_t max_size){
	char *_key = (char *)key;
	return bkdr_hash(_key) % max_size;
}




/**
 * @brief 初始化哈希表
 *
 * @param ht 哈希表指针
 */
inline void hash_table_init(struct hash_table *ht, hash_func comp, hashf_func hf){
	ht->data = (struct list_head *)kmalloc(HASH_SIZE);
	ht->size = HASH_SIZE / sizeof(struct list_head);
	uint32_t cnt = ht->size;
	while(cnt--){
		LIST_HEAD_INIT(ht->data[cnt]);
	}
	ht->compare = comp;
	ht->hashf = hf;
}

/**
 * @brief 哈希表插入
 *
 * @param ht 哈希表指针
 * @param elem 待插入的哈希元素
 *
 */
inline void hash_table_insert(struct hash_table *ht, struct list_head *elem, void *key){
	uint32_t idx = ht->hashf(key, ht->size);
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
inline struct list_head *hash_table_find(struct hash_table *ht, void *key){
	uint32_t idx = ht->hashf(key, ht->size);
	struct list_head *head = &ht->data[idx];
	struct list_head *cur = head->next;

	while(cur != head){
		//这里性能可能有问题
		if(ht->compare(cur, key))
			return cur;
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
	memset(ht->data, 0, HASH_SIZE);
}

/**
 * 释放哈希表
 */
inline void hash_table_del(struct hash_table *ht){

	kfree(ht->data);
	ht->size = 0;
	ht->data = NULL;
}

