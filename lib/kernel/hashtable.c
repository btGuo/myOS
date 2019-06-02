#include "hashtable.h"
#include "memory.h"

#define hashf(ht, x) ((x) % (ht->size))  ///< 哈希映射函数

#define quadprobe(x, i) ((x) + (i) * (i)) ///< 平方探测函数

#define PROBE_CNT 5  ///< 探测次数

/**
 * @brief 初始化哈希表
 *
 * @param ht 哈希表指针
 * @param size 哈希表大小
 */
void hash_table_init(struct hash_table *ht, uint32_t size){
	ht->data = (struct hash_elem *)sys_malloc(size * sizeof(struct hash_elem));
	ht->size = size;
	memset(ht->data, 0, size * sizeof(struct hash_elem));
}

/**
 * @brief 哈希表插入
 *
 * @param ht 哈希表指针
 * @param elem 待插入的哈希元素
 *
 * @return 插入是否成功
 */
inline bool hash_table_insert(struct hash_table *ht, struct hash_elem *elem){
	uint32_t idx = hashf(ht, elem->key);
	uint32_t probe_cnt = 1;
	while(ht->data[idx].val == 0 && probe_cnt < PROBE_CNT){
		idx = quadprobe(idx, probe_cnt);
		idx % ht->size;
		++probe_cnt;
	}
	if(probe_cnt == PROBE_CNT)
		return false;
	ht->data[idx].val = elem->val;
	ht->data[idx].key = elem->key;
	return true;
}
/**
 * @brief 哈希表查找
 *
 * @param ht 哈希表指针
 * @param key 待查找的关键字
 *
 * @return 返回值
 * 	@retval NULL 查找为空
 */
inline void *hash_table_find(struct hash_table *ht, uint32_t key){
	uint32_t idx = hashf(ht, key);
	uint32_t probe_cnt = 1;
	while(ht->data[idx].key != key && probe_cnt < PROBE_CNT){
		idx = quadprobe(idx, probe_cnt);
		idx % ht->size;
		++probe_cnt;
	}
	if(probe_cnt == PROBE_CNT)
		return NULL;
	return ht->data[idx].val;
}
/**
 * 清空哈希表
 */
void hash_table_clear(struct hash_table *ht){
	memset(data, 0, sizeof(struct hash_elem) * ht->size);
}
