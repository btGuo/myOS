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
bool try_expend_heap();
bool try_expend_stack();
bool is_in_vm_area(uint32_t vaddr);
struct vm_area *vm_area_alloc(uint32_t saddr, uint32_t size);
void vm_area_add(struct vm_area *vm);
pid_t sys_fork(void);

int32_t sys_execv(const char *path, const char **argv);
#endif
