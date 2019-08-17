#ifndef _VM_AREA_H
#define _VM_AREA_H

#include "list.h"
#include "stdint.h"
#include "global.h"

enum vm_area_type{
	VM_UPWARD,
	VM_DOWNWARD,
	VM_FIX
};

/**
 * 虚拟内存区域
 */
struct vm_area{
	struct list_head vm_tag;  ///< 用于vm_struct.vm_list
	enum vm_area_type vm_type;
	uint8_t  ref_cnt;   ///< 引用计数
	uint32_t start_addr; ///< 起始地址
	uint32_t size;  ///< 字节数
};

/**
 * 进程虚拟内存管理
 */
struct vm_struct{
	uint32_t vaddr_start;  //TODO 初始化
	struct list_head *vm_list;  ///< 这里得用指针
	struct vm_area *vm_stack;
	struct vm_area *vm_heap;
};	

#endif
