#include "process.h"
#include "string.h"
#include "global.h"
#include "memory.h"
#include "tss.h"
#include "interrupt.h"
#include "print.h"

extern void intr_exit(void);
extern struct task_struct *curr;
extern struct list_head thread_ready_list;
extern struct list_head thread_all_list;

void start_process(void *filename_){
	put_str("in start_process\n");
	void *function = filename_;

	curr->self_kstack += sizeof(struct thread_stack);
	struct intr_stack *proc_stack = (struct intr_stack *)curr->self_kstack;
	memset((void *)proc_stack, 0, sizeof(struct intr_stack));

	proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
	proc_stack->cs = SELECTOR_U_CODE;
	proc_stack->ss = SELECTOR_U_DATA;

	proc_stack->eip = function;
	proc_stack->eflags = EFLAGS_MBS | EFLAGS_IF_ON | EFLAGS_IOPL_0;
	proc_stack->esp = (void *) ((uint32_t)get_a_page(PF_USER, \
			   	USER_STAKC_VADDR) + PG_SIZE);

	asm volatile("movl %0, %%esp; \
				  add $4, %%esp; 	\
				  popal;   \
				  mov $0x33, %%ax;   \
				  mov %%ax, %%fs;   \
				  mov %%ax, %%gs;   \
				  mov %%ax, %%es;   \
				  mov %%ax, %%ds;   \
				  add $32, %%esp; \
				  add $4, %%esp;  \
				  iret;"::"g"(proc_stack):"memory");
}

void page_dir_activate(struct task_struct *p_thread){
	uint32_t paddr = 0x100000;
	if(p_thread->pg_dir){
		paddr = addr_v2p((uint32_t)p_thread->pg_dir);
	}
	asm volatile("movl %0, %%cr3"::"r"(paddr):"memory");
}

void process_activate(struct task_struct *p_thread){
	page_dir_activate(p_thread);
	if(p_thread->pg_dir){
		update_tss_esp(p_thread);
	}
}

uint32_t *create_page_dir(void){
	uint32_t *vaddr = get_kernel_pages(1);
	if(vaddr == NULL){
		console_write("create page dir failed, no more space!\n");
	}

	memcpy((uint32_t*)((uint32_t)vaddr + 0x300 * 4), \
			(uint32_t*)(0xfffff000 + 0x300 * 4), 1024);

	uint32_t paddr = addr_v2p((uint32_t)vaddr);
	vaddr[1023] = paddr | 0x7;
	return vaddr;
}

void create_user_vaddr_bitmap(struct task_struct *user_prog){
	user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;
	//以页为单位申请空间
	uint32_t cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) /\
		   	PG_SIZE / 8, PG_SIZE);
	printk("%d\n", cnt);
	user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(cnt);
	user_prog->userprog_vaddr.vaddr_bitmap.byte_len = cnt;
	bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

void process_execute(void *filename, char *name){
	struct task_struct *thread = get_kernel_pages(1);
	init_thread(thread, name, default_prio);
	create_user_vaddr_bitmap(thread);
	thread_create(thread, start_process, filename);
	thread->pg_dir= create_page_dir();
	
	enum intr_status old_stat = intr_disable();
	list_add_tail(&thread->ready_tag, &thread_ready_list);
	list_add_tail(&thread->all_tag, &thread_all_list);
	intr_set_status(old_stat);
}

