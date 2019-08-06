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

struct vm_area{
	struct list_head vm_tag;
	enum vm_area_type vm_type;
	uint32_t start_addr;
	uint32_t size;
};

struct vm_struct{
	struct list_head vm_list;
	struct vm_area vm_stack;
	struct vm_area vm_heap;
};	

#endif
