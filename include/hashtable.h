#ifndef __LIB_KERNEL_HASHTABLE_H
#define __LIB_KERNEL_HASHTABLE_H

#include "stdint.h"
#include "buffer_head.h"


/**
 * @brief 哈希表，开哈希，数据域存放链表头
 */
struct hash_table{
	uint32_t length;   ///< 哈希表槽个数
	struct list_head *data;  ///< 数据指针
};


#endif
