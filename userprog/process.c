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
 * 初始化进程内存区
 */
void vm_struct_init(){

	struct vm_area *area = NULL;

	//堆线性区
	area = &curr->vm_struct.vm_heap;
	area->start_addr = USER_HEAP_VADDR;
	area->size = (1 << 20);
	area->vm_type = VM_DOWNWARD;

	//栈线性区
	area = &curr->vm_struct.vm_stack;
	area->start_addr = USER_STAKC_VADDR;
	area->size = (1 << 20);
	area->vm_type = VM_UPWARD;

	//内核线性区
	area = &curr->vm_struct.vm_kernel;
	area->start_addr = kmm.vm_kernel.start_addr;
	area->size = kmm.vm_kernel.size;
	area->vm_type = kmm.vm_kernel.vm_type;

	LIST_HEAD_INIT(curr->vm_struct.vm_list);
}

/**
 * 进程开始，这里任务已经切换，已经是新进程地址空间
 */
void start_process(void *filename_){
	
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
	proc_stack->esp = (void *)USER_STAKC_VADDR;

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

