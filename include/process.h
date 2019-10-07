#ifndef __PROCESS_H
#define __PROCESS_H

#define USER_STAKC_VADDR 0xbff00000  ///< 用户栈地址
#define USER_VADDR_START 0x8048000    ///< 用户内存起始地址
#define USER_HEAP_VADDR 0xbf800000   ///< 堆区默认起始地址

#include "thread.h"
#include "vm_area.h"
#define default_prio 32


void process_execute(void *filename, char *name);
void process_activate(struct task_struct *p_thread);
uint32_t *create_page_dir(void);

bool is_in_vm_area(uint32_t vaddr);
struct vm_area *vm_area_alloc(uint32_t saddr, uint32_t size, enum vm_area_type vm_type);
void vm_area_add(struct vm_struct *vm_s, struct vm_area *vm);

int  vm_list_ctor(struct vm_struct *vm_s);
void vm_list_dtor(struct vm_struct *vm_s);
int  vm_list_copy(struct vm_struct *vm_s);

void vm_struct_update(struct vm_struct *vm_s);
int  vm_struct_ctor(struct vm_struct *vm_s);
void vm_struct_dtor(struct vm_struct *vms);

pid_t sys_fork(void);
#endif
