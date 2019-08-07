#include "process.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "tss.h"
#include "interrupt.h"
#include "print.h"
#include "vm_area.h"

extern void intr_exit(void);
extern struct task_struct *curr;
extern struct list_head thread_ready_list;
extern struct list_head thread_all_list;

/**
 * 增加用户堆
 */
bool try_expend_heap(){
	struct vm_area *heap = &curr->vm_struct.vm_heap;
	struct vm_area *stack = &curr->vm_struct.vm_stack;
	//这里比较暴力，直接扩大1M
	heap->size += (1 << 20);
	if(heap->start_addr + heap->size > stack->start_addr)
		return false;
	return true;
}

/**
 * 增加用户栈
 */
bool try_expend_stack(){
	struct vm_area *heap = &curr->vm_struct.vm_heap;
	struct vm_area *stack = &curr->vm_struct.vm_stack;
	//这里比较暴力，直接扩大1M
	stack->start_addr -= (1 << 20);
	stack->size += (1 << 20);
	if(heap->start_addr + heap->size > stack->start_addr)
		return false;
	return true;
}

/**
 * 判断给定虚拟地址是否在线性区中
 */
bool is_in_vm_area(uint32_t vaddr){
	struct list_head *head = &curr->vm_struct.vm_list;
	struct list_head *walk = head->next;
	struct vm_area *vm = NULL;
	while(walk != head){
		vm = list_entry(struct vm_area, vm_tag, walk);
		if(vaddr >= vm->start_addr && vaddr < vm->start_addr + vm->size)
			return true;
		walk = walk->next;
	}
	return false;
}

struct vm_area *vm_area_alloc(uint32_t saddr, uint32_t size){
	struct vm_area *vm = (struct vm_area *)kmalloc(sizeof(struct vm_area));

	if(!vm){
		return NULL;
	}

	//saddr &= 0xfffff000;
	//默认fix
	vm->vm_type = VM_FIX;
	vm->size = size;
	vm->start_addr = saddr;
	return vm;
}

void vm_area_add(struct vm_area *vm){
	list_add_tail(&vm->vm_tag, &curr->vm_struct.vm_list);
}
	
/**
 * 初始化进程内存区
 */
void vm_struct_init(){

	LIST_HEAD_INIT(curr->vm_struct.vm_list);
	struct vm_area *area = NULL;

	//堆线性区
	area = &curr->vm_struct.vm_heap;
	area->start_addr = USER_HEAP_VADDR;
	area->size = (1 << 20);
	area->vm_type = VM_DOWNWARD;
	list_add(&area->vm_tag, &curr->vm_struct.vm_list);

	//栈线性区
	area = &curr->vm_struct.vm_stack;
	area->start_addr = USER_STAKC_VADDR;
	area->size = (1 << 20);
	area->vm_type = VM_UPWARD;
	list_add(&area->vm_tag, &curr->vm_struct.vm_list);

}

/**
 * 进程开始，这里任务已经切换，已经是新进程地址空间
 */
void start_process(void *filename_){
	
//	printk("start process\n");
	vm_struct_init();
	void *function = filename_;

	curr->self_kstack += sizeof(struct thread_stack);
	struct intr_stack *proc_stack = (struct intr_stack *)curr->self_kstack;
	memset((void *)proc_stack, 0, sizeof(struct intr_stack));

	proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
	proc_stack->cs = SELECTOR_U_CODE;
	proc_stack->ss = SELECTOR_U_DATA;

	proc_stack->eip = function;
	proc_stack->eflags = EFLAGS_MBS | EFLAGS_IF_ON | EFLAGS_IOPL_0;
	//分配用户栈
	proc_stack->esp = (void *)((uint32_t)get_a_page(PF_USER, 0xbffff000) + PG_SIZE);

	asm volatile("movl %0, %%esp; jmp intr_exit;"\
			::"g"(proc_stack):"memory");
}

/**
 * 激活页目录
 */
void page_dir_activate(struct task_struct *p_thread){
	uint32_t paddr = 0x100000;
	if(p_thread->pg_dir){
		paddr = addr_v2p((uint32_t)p_thread->pg_dir);
	}
	asm volatile("movl %0, %%cr3"::"r"(paddr):"memory");
}

/**
 * 激活进程，在schedule中被调用
 */
void process_activate(struct task_struct *p_thread){
	page_dir_activate(p_thread);
	if(p_thread->pg_dir){
		update_tss_esp(p_thread);
	}
}

/**
 * 创建新进程页目录
 */
uint32_t *create_page_dir(void){
	uint32_t *vaddr = get_kernel_pages(1);
	if(vaddr == NULL){
		console_write("create page dir failed, no more space!\n");
	}

	//复制高端内存页目录
	memcpy((uint32_t*)((uint32_t)vaddr + 0x300 * 4), \
			(uint32_t*)(0xfffff000 + 0x300 * 4), 1024);

	uint32_t paddr = addr_v2p((uint32_t)vaddr);
	vaddr[1023] = paddr | 0x7;
	return vaddr;
}

/**
 * 创建新进程，分配相关资源
 */
void process_execute(void *filename, char *name){
	struct task_struct *thread = get_kernel_pages(1);
	init_thread(thread, name, default_prio);
//	create_user_vaddr_bitmap(thread);
	thread_create(thread, start_process, filename);
	thread->pg_dir= create_page_dir();
	//初始化虚拟地址
	thread->userprog_vaddr.vaddr_start = USER_VADDR_START;
	
	enum intr_status old_stat = intr_disable();
	list_add_tail(&thread->ready_tag, &thread_ready_list);
	list_add_tail(&thread->all_tag, &thread_all_list);
	intr_set_status(old_stat);
}

