#include <fork.h>
#include <global.h>
#include <thread.h>
#include <list.h>
#include <interrupt.h>
#include <file.h>
#include <memory.h>
#include <file.h>
#include <process.h>
#include <string.h>
#include <pipe.h>
#include <inode.h>

extern void intr_exit(void);
//TODO 目前fork时vm_struct 是共享的，更新时会一起更新。

/**
 * 复制任务块
 */
static int32_t copy_pcb(struct task_struct *child, struct task_struct *parent){

	memcpy(child, parent, PG_SIZE);
	child->pid = fork_pid();
	child->elapsed_ticks = 0;
	child->status = TASK_READY;
	child->ticks = child->priority;
	child->par_pid = parent->pid;
	child->par = parent;
	list_add_tail(&child->par_tag, &parent->children);
	LIST_HEAD_INIT(child->children);
	//增加引用计数
	vm_struct_update(&child->vm_struct);

	return 0;
}

/**
 * 构建子进程内核栈
 */
static int32_t build_child_stack0(struct task_struct *child){

	struct intr_stack *i_sta = (struct intr_stack *)((uint32_t)child + \
			PG_SIZE - sizeof(struct intr_stack));

	//返回值为0
	i_sta->eax = 0;

	uint32_t *p_ret = (uint32_t *)i_sta - 1;
	uint32_t *p_esi = (uint32_t *)i_sta - 2;
	uint32_t *p_edi = (uint32_t *)i_sta - 3;
	uint32_t *p_ebx = (uint32_t *)i_sta - 4;
	uint32_t *p_ebp = (uint32_t *)i_sta - 5;

	//看switch_to
	*p_ret = (uint32_t)intr_exit;
	*p_ebp = *p_edi = *p_esi = *p_ebx = 0;
	child->self_kstack = p_ebp;

	return 0;
}

/**
 * 增加引用次数
 */
static void up_inode(struct task_struct *thread){

	struct file *fp = NULL;
	struct fext_inode_m *inode = NULL;
	int32_t idx = 0;

	while(idx < MAXL_OPENS){
		
		fp = curr->fd_table[idx];
		if(fp){
			++fp->fd_count;
			inode = fp->fd_inode;

			if(S_ISREG(inode->i_type))
				inode->i_open_cnts++;
		}
		idx++;
	}
}

static int32_t copy_process(struct task_struct *child, struct task_struct *parent){

#ifdef DEBUG
	printk("copy_process\n");
#endif
	copy_pcb(child, parent);
	child->pg_dir = create_page_dir();
	if(child->pg_dir == NULL)
		return -1;

#ifdef DEBUG
	printk("child pg_dir %x\n", child->pg_dir);
#endif
	copy_page_table(child->pg_dir);		

	build_child_stack0(child);
	up_inode(child);
#ifdef DEBUG
	printk("copy_process done\n");
#endif
	return 0;
}

pid_t sys_fork(void){

#ifdef DEBUG
	printk("sys_fork\n");
#endif
	struct task_struct *parent = curr;
	struct task_struct *child = get_kernel_pages(1);
	if(child == NULL)
		return -1;

	ASSERT(intr_get_status() == INTR_OFF && \
			parent->pg_dir != NULL);

	if(copy_process(child, parent) == -1)
		return -1;

	list_add_tail(&child->ready_tag, &thread_ready_list);
	list_add_tail(&child->all_tag, &thread_all_list);
#ifdef DEBUG
	printk("sys_fork done\n");
#endif
	return child->pid;
}

