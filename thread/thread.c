#include <string.h>
#include <memory.h>
#include <interrupt.h>
#include <thread.h>
#include <process.h>
#include <global.h>
#include <fs.h>

struct task_struct *main_thread;
struct task_struct *curr;
struct task_struct *idle_thread;
struct task_struct *init_prog;

LIST_HEAD(thread_all_list);
LIST_HEAD(thread_ready_list);
struct pid_pool pid_pool;
extern void init(void);

void print_thread(struct task_struct *task){
	printk("vaddr : %x\n", (uint32_t)task);
	printk("priority : %d\n", task->priority);
	printk("name : %s\n", task->name);
}

static void kernel_thread(thread_func *function, void *func_arg){
	printk("kernel thread\n");
	//这里打开中断
	intr_enable();
	function(func_arg);
}

/**
 * 初始化pid池
 */
static void pid_pool_init(void){

	pid_pool.pid_start = 1;
	mutex_lock_init(&pid_pool.lock);
	pid_pool.bmp.byte_len = PG_SIZE;
	pid_pool.bmp.bits = get_kernel_pages(1);
	bitmap_init(&pid_pool.bmp);
}

/**
 * 分配pid
 */
pid_t allocate_pid(void){
	mutex_lock_acquire(&pid_pool.lock);
	int32_t idx = bitmap_scan(&pid_pool.bmp, 1);
	bitmap_set(&pid_pool.bmp, idx, 1);
	mutex_lock_release(&pid_pool.lock);
	return idx + pid_pool.pid_start;
}

pid_t fork_pid(void){
	return allocate_pid();
}

/**
 * 释放pid
 */
void release_pid(pid_t pid){
	mutex_lock_acquire(&pid_pool.lock);
	uint32_t idx = pid - pid_pool.pid_start;
	bitmap_set(&pid_pool.bmp, idx, 0);
	mutex_lock_release(&pid_pool.lock);
}

static void idle(void UNUSED){
#ifdef DEBUG
	printf("idle\n");
#endif
	while(1){
		thread_block(TASK_BLOCKED);
		asm volatile ("sti;hlt" : : : "memory");
	}
}

/**
 * 初始化线程栈
 */
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

/**
 * 初始化任务控制块
 */
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
	pthread->pid = allocate_pid();

	if(root_fs){

		pthread->root_i = root_fs->root_i;
		pthread->cwd_i = root_fs->root_i;
	}

	pthread->fd_table[0] = 0;
	pthread->fd_table[1] = 1;
	pthread->fd_table[2] = 2;
	uint8_t idx = 3;
	while(idx < MAX_FILES_OPEN_PER_PROC){
		pthread->fd_table[idx] = -1;
		++idx;
	}
	pthread->stack_magic = STACK_MAGIC;
}

/**
 * 创建新线程
 */
struct tack_struct* thread_start(char *name, int prio, \
		thread_func function, void *func_arg){

	struct task_struct *thread = (struct task_struct*)get_kernel_pages(1);
	init_thread(thread, name, prio);
	thread_create(thread, function, func_arg);
	list_add_tail(&thread->all_tag, &thread_all_list);	
	list_add_tail(&thread->ready_tag, &thread_ready_list);
	return thread; 
}

/**
 * 阻塞当前任务, 需要重新调度
 */
void thread_block(enum task_status stat){
	ASSERT(stat == TASK_BLOCKED || stat == TASK_WATTING ||
			stat == TASK_HANGING);
	enum intr_status old_stat = intr_disable();
	curr->status = stat;
	schedule();
	intr_set_status(old_stat);
}

/**
 * 唤醒任务
 */
void thread_unblock(struct task_struct *nthread){
	enum intr_status old_stat = intr_disable();
	list_add(&nthread->ready_tag, &thread_ready_list);
	nthread->status = TASK_READY;
	intr_set_status(old_stat);
}

static void make_main_thread(void){
	main_thread = (struct task_struct *)MAIN_PCB;
	init_thread(main_thread, "main", 31);
	main_thread->pg_dir = 0xc0200000;
	//不需要加入read队列
	list_add_tail(&main_thread->all_tag, &thread_all_list);
	curr = main_thread;
}

void thread_yield(void){
	enum intr_status old_stat = intr_disable();
	list_add_tail(&curr->ready_tag, &thread_ready_list);
	curr->status = TASK_READY;
	schedule();
	intr_set_status(old_stat);
}

void thread_exit(struct task_struct *over, bool sched){

	intr_disable();
	list_del(&over->ready_tag);
	list_del(&over->all_tag);
	list_del(&over->par_tag);

	release_pid(over->pid);
	if(over != main_thread){
		pfree(addr_v2p(over));
	}

	if(sched){
		schedule();
		PANIC("should not be here\n");
	}
}

/**
 * 调度
 */
void schedule(){
	if(curr->status == TASK_RUNNING ){
		//重新设置优先级，加入队列
		curr->ticks = curr->priority;
		curr->status = TASK_READY;
		list_add_tail(&curr->ready_tag, &thread_ready_list);
	}else{

	}
	struct task_struct *prev = curr;
	//如果队列为空，唤醒idle
	if(list_empty(&thread_ready_list)){
		thread_unblock(idle_thread);
	}
	//从就绪队列中拿出第一个任务
	curr = list_entry(struct task_struct, ready_tag, thread_ready_list.next);
	list_del(&curr->ready_tag);
	curr->status = TASK_RUNNING;
	//激活页表
	process_activate(curr);	
	switch_to(prev, curr);
}
				
void thread_init(){
	printk("init_thread start\n");
	pid_pool_init();
	
	process_execute(init, "init");
	make_main_thread();
	idle_thread = thread_start("idle", 10, idle, NULL);

	printk("init_thread done\n");
}
