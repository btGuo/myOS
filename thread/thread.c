#include "string.h"
#include "memory.h"
#include "interrupt.h"
#include "thread.h"
#include "process.h"

struct task_struct *main_thread;
struct task_struct *curr;
LIST_HEAD(thread_all_list);
LIST_HEAD(thread_ready_list);

void print_thread(struct task_struct *task){
	put_str("vaddr :");put_int((uint32_t)task);put_char('\n');
	put_str("self_kstack :");put_int((uint32_t)task->self_kstack);put_char('\n');
	put_str("priority :");put_int(task->priority);put_char('\n');
	put_str("name :");put_str(task->name);put_char('\n');
}

static void kernel_thread(thread_func *function, void *func_arg){
	//这里打开中断
	intr_enable();
	function(func_arg);
}

void thread_create(struct task_struct *pthread, thread_func function, void *func_arg){
	pthread->self_kstack -= sizeof(struct intr_stack);
	pthread->self_kstack -= sizeof(struct thread_stack);
	struct thread_stack *kthread_stack = (struct thread_stack*)pthread->self_kstack;
	kthread_stack->eip = kernel_thread;
	kthread_stack->function = function;
	kthread_stack->func_arg = func_arg;	
	kthread_stack->ebp = kthread_stack->ebx = \
	kthread_stack->esi = kthread_stack->edi = 0;
}

void init_thread(struct task_struct *pthread, char *name, int prio){
	memset(pthread, 0, sizeof(struct task_struct));
	strcpy(pthread->name, name);
	if(pthread == main_thread){
		pthread->status = TASK_RUNNING;
	}else{
		pthread->status = TASK_READY;
	}
	pthread->elapsed_ticks = 0;
	pthread->pg_dir = NULL;
	pthread->ticks = prio;
	pthread->priority = prio;
	pthread->self_kstack = (uint32_t*)((uint32_t)pthread + PG_SIZE);
	pthread->stack_magic = STACK_MAGIC;
}

struct tack_struct* thread_start(char *name, int prio, \
		thread_func function, void *func_arg){
	struct task_struct *thread = (struct task_struct*)get_kernel_pages(1);
	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);
	list_add_tail(&thread->all_tag, &thread_all_list);	
	list_add_tail(&thread->ready_tag, &thread_ready_list);
	return thread; 
}
//当前任务被阻塞, 需要重新调度
void thread_block(enum task_status stat){
	ASSERT(stat == TASK_BLOCKED || stat == TASK_WATTING ||
			stat == TASK_HANGING);
	enum intr_status old_stat = intr_disable();
	curr->status = stat;
	schedule();
	intr_set_status(old_stat);
}

void thread_unblock(struct task_struct *nthread){
	enum intr_status old_stat = intr_disable();
	list_add(&nthread->ready_tag, &thread_ready_list);
	nthread->status = TASK_READY;
	intr_set_status(old_stat);
}

static void make_main_thread(void){
	main_thread = (struct task_struct *)MAIN_PCB;
	init_thread(main_thread, "main", 31);
	list_add_tail(&main_thread->all_tag, &thread_all_list);
	list_add_tail(&main_thread->ready_tag, &thread_ready_list);
	curr = main_thread;
}

void schedule(){
	if(curr->status == TASK_RUNNING ){
		curr->ticks = curr->priority;
		curr->status = TASK_READY;
		list_del(&curr->ready_tag);
		list_add_tail(&curr->ready_tag, &thread_ready_list);
	}else{

	}
	struct task_struct *prev = curr;
	curr = list_entry(struct task_struct, ready_tag, thread_ready_list.next);
	curr->status = TASK_RUNNING;
	process_activate(curr);	
	switch_to(prev, curr);
}
				
void thread_init(){
	put_str("init_thread start\n");
	make_main_thread();
	put_str("init_thread done\n");
}
