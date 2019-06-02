#ifndef __LIB_KERNEL_HASHTABLE_H
#define __LIB_KERNEL_HASHTABLE_H

#include "stdint.h"

/**
 * 哈希表元素
 */
struct hash_elem{
	uint32_t key;  ///< 关键字
	uint32_t *val; ///< 值
};
/**
 * @brief 哈希表
 */
struct hash_table{
	uint32_t size;   ///< 哈希表大小
	struct hash_elem *data;  ///< 数据指针
};


#endif
