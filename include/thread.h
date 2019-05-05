#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include"stdint.h"
#include"list.h"

typedef void thread_func(void*);
void schedule();

#define PG_SIZE 4096
#define MAIN_PCB 0xc009e000
#define STACK_MAGIC 0x21436587

enum task_status{
	TASK_RUNNING,
	TASK_READY,
	TASK_BLOCKED,
	TASK_WATTING,
	TASK_HANGING,
	TASK_DIED
};

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
	uint32_t stack_magic;
};

struct task_struct *main_thread;
struct task_struct *curr_thread;
static LIST_HEAD(thread_all_list);
static LIST_HEAD(thread_ready_list);

#endif
