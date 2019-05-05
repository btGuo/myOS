#include"thread.h"
#include"stdint.h"
#include"string.h"
#include"global.h"
#include"memory.h"

static void kernel_thread(thread_func *function, void *func_arg){
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
	struct task_struct *thread = get_kernel_pages(1);
	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);
	list_add_tail(&thread->all_tag, &thread_all_list);	
	list_add_tail(&thread->ready_tag, &thread_ready_list);
	return thread; 
}

static void make_main_thread(void){
	main_thread = (struct task_struct *)MAIN_PCB;
	init_thread(main_thread, "idle", 31);
	list_add_tail(&main_thread->all_tag, &thread_all_list);
	list_add_tail(&main_thread->ready_tag, &thread_ready_list);
	curr_thread = main_thread;
}

void schedule(){
	if(curr_thread->status == TASK_RUNNING ){
		curr_thread->ticks = curr_thread->priority;
		curr_thread->status = TASK_READY;
		list_del(&curr_thread->ready_tag);
		list_add_tail(&thread_ready_list, &curr_thread->ready_tag);
	}else{
	}
	struct task_struct *prev = curr_thread;
	curr_thread = list_entry(struct task_struct, ready_tag, thread_ready_list.next);
	curr_thread->status = TASK_RUNNING;
	switch_to(prev, curr_thread);
}
				
void thread_init(){
	put_str("init_thread start\n");
	make_main_thread();
	put_str("init_thread done\n");
}
