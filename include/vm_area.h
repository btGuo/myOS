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
	uint32_t start_addr; ///< 起始地址
	uint32_t size;  ///< 字节数
};

/**
 * 进程虚拟内存管理
 */
struct vm_struct{
	uint32_t vaddr_start;  //TODO 初始化
	struct list_head *vm_list;  ///< 这里得用指针
	struct vm_area vm_stack;   ///< 栈线性区，不在vm_list中
	struct vm_area vm_heap;    ///< 堆先行区，不在vm_list中
	uint32_t heap_ptr;       ///< 当前栈指针
	uint32_t *vml_refcnt;   ///< vm_list 引用次数
};	

#endif
