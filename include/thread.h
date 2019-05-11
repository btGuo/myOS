#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include"stdint.h"
#include"list.h"

typedef void thread_func(void*);
void schedule();
void print_thread();
void thread_block();
void thread_unblock();

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



#endif
