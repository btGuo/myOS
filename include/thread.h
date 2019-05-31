#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"

typedef void thread_func(void*);
void schedule();
void print_thread();
void thread_block();
void thread_unblock();
struct tack_struct* thread_start(char *name, int prio, thread_func function, void *func_arg);
void init_thread(struct task_struct *pthread, char *name, int prio);
void thread_yield(void);
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg);

void thread_init();
typedef uint32_t pid_t;

#define PG_SIZE 4096
#define MAIN_PCB 0xc009e000
#define STACK_MAGIC 0x21436587

struct intr_stack{
	uint32_t vec_no;
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t esp_dummy;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;

	uint32_t err_code;
	void (*eip)(void);
	uint32_t cs;
	uint32_t eflags;
	void *esp;
	uint32_t ss;
};

struct thread_stack{
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edi;
	uint32_t esi;
	void (*eip)(thread_func *func, void *func_arg);

	void (*unused_retaddr);
	thread_func *function;
	void *func_arg;
};

enum task_status{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WATTING,
	TASK_HANGING,
	TASK_DIED
};

#define MAX_FILES_OPEN_PER_PROC 6

/*
 * 程序控制任务块
 * self_kstack 		内核栈指针
 * status 			任务当前状态
 * priority  		优先级
 * ticks 			滴答数
 * elapsed_ticks   	总共占用了多少cpu时间		
 * all_tag 			所有任务链表标签
 * ready_tag 		就绪任务链表标签
 * name 			任务名称
 * pg_dir 			页目录地址
 * stack_magic 		魔数, 标记用
 */
struct task_struct{
	uint32_t *self_kstack;
	enum task_status status;
	uint8_t priority;
	uint8_t ticks;
	uint32_t elapsed_ticks;
	struct list_head ready_tag;
	struct list_head all_tag;
	char name[16];
	uint32_t *pg_dir;
	struct virtual_addr userprog_vaddr;
	pid_t pid;
	struct mem_block_desc u_block_desc[DESC_CNT];
	uint32_t stack_magic;
	int32_t fd_table[MAX_FILES_OPEN_PER_PROC];
};



#endif
