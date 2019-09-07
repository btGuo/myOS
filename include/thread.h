#ifndef __THREAD_THREAD_H
#define __THREAD_THREAD_H
#include "stdint.h"
#include "list.h"
#include "memory.h"
#include "vm_area.h"

#define PG_SIZE 4096         ///< 页大小
#define MAIN_PCB 0xc009e000     ///< 主线程pcb所在地址
#define STACK_MAGIC 0x21436587     ///< 栈魔数，随便定义的
#define MAX_FILES_OPEN_PER_PROC 6    ///< 每进程最大打开文件数
#define MAX_PID 32768    ///< 最大进程数

typedef int32_t pid_t;
typedef void thread_func(void*);
/**
 * 中断栈，切换任务时保存上下文信息
 */
struct intr_stack{
	uint32_t pad;  ///< 对齐用
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

/**
 * 线程栈
 */
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

/**
 * 进程状态枚举
 */
enum task_status{
	TASK_RUNNING,          ///<  运行
	TASK_READY,            ///<  已准备好，随时可运行
	TASK_BLOCKED,          ///<  被阻塞，等待唤醒
	TASK_WATTING,          ///<  等待
	TASK_HANGING,          ///<  挂起
	TASK_DIED              ///<  死亡
};

#define TASK_NAME_LEN 16

/**
 * 程序控制任务块
 */
struct task_struct{
	uint32_t *self_kstack;       	 ///<  内核栈指针
	enum task_status status;     	 ///<  当前任务状态
	uint8_t priority;                ///<  优先级
	uint8_t ticks;                   ///< 运行滴答数
	uint32_t elapsed_ticks;          ///<   总共占用的cpu时间

	struct list_head ready_tag;       ///<  用于任务队列的链表节点
	struct list_head all_tag;         ///<  用于所有任务队列的链表节点
	struct list_head children;        ///< 子进程队列
	struct list_head par_tag;         ///< 用于父进程队列

	char name[TASK_NAME_LEN];                    ///<  任务名称
	uint32_t *pg_dir;                 ///<  页目录指针
	pid_t pid;                          ///<  任务号
	pid_t par_pid;                      ///< 父进程号
	struct task_struct *par;        ///< 父进程
	struct inode_info *root_i;      ///< 所在根inode
	struct inode_info *cwd_i;      ///< 当前目录inode
	int32_t fd_table[MAX_FILES_OPEN_PER_PROC];    ///< 文件号表
	struct vm_struct vm_struct;   		      ///< 内存区管理
	int8_t exit_status;        
	uint32_t stack_magic;                         ///< 魔数，标记是否越界
};

/**
 * pid池
 */
struct pid_pool{
	struct bitmap bmp;
	struct mutex_lock lock;
	uint32_t pid_start;
};

extern struct task_struct *curr;
extern struct task_struct *init_prog;
extern struct list_head thread_all_list;
extern struct list_head thread_ready_list;

void schedule();
void print_thread();
void thread_block();
void thread_unblock();
struct tack_struct* thread_start(char *name, int prio, thread_func function, void *func_arg);
void init_thread(struct task_struct *pthread, char *name, int prio);
void thread_yield(void);
void thread_create(struct task_struct *pthread, thread_func function, void *func_arg);
void thread_exit(struct task_struct *over, bool sched);
pid_t fork_pid(void);

void thread_init();
#endif
