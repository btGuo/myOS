#ifndef __LIB_KERNEL_HASHTABLE_H
#define __LIB_KERNEL_HASHTABLE_H

#include "stdint.h"

typedef bool (*hash_func) (struct list_head *, uint32_t);

/**
 * @brief 哈希表，开哈希，数据域存放链表头
 */
struct hash_table{
	uint32_t size;   ///< 哈希表槽个数
	struct list_head *data;  ///< 数据指针
	hash_func compare;
};


#endif
