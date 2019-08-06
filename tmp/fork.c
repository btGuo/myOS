#include "fork.h"
#include "global.h"
#include "thread.h"
#include "list.h"
#include "interrupt.h"
#include "file.h"
#include "memory.h"

extern void intr_exit(void);
extern struct task_struct *curr;
extern struct list_head thread_ready_list;
extern struct list_head thread_all_list;

static int32_t copy_pcb(struct task_struct *child, struct task_struct *parent){

	memcpy(child, parent, PG_SIZE);
	child->pid = fork_pid();
	child->elapsed_ticks = 0;
	child->status = TASK_READY;
	child->ticks = child->priority;
	child->par_pid = parent->pid;
	block_desc_init(child->u_block_desc);

	return 0;
}

static int32_t build_child_stack(struct task_struct *child){

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

//增加引用次数
static void up_inode(struct task_struct *thread){

	int32_t l_fd = 3, g_fd = 0;
	while(l_fd < MAX_FILES_OPEN_PER_PROC){
		g_fd = thread->fd_table[l_fd];
		if(g_fd != -1){
			file_table[g_fd].fd_inode->i_open_cnts++;
		}
		++l_fd;
	}
}

static int32_t copy_process(struct task_struct *child, struct task_struct *parent){

	copy_pcb(child, parent);
	child->pg_dir = create_page_dir();
	if(child->pg_dir == NULL)
		return -1;

	copy_page_table(child->pg_dir);		

	build_child_stack(child);
	up_inode(child);
	return 0;
}

pid_t sys_fork(void){

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

	return child->pid;
}





