#include <stdint.h>
#include <thread.h>
#include <kernelio.h>
#include <fs_sys.h>
#include <process.h>
#include <thread.h>

static inline void do_vm_area(struct vm_area *vm){

	uint32_t size  = 0;
	uint32_t off   = vm->start_addr & 0x00000fff;  //页框
	uint32_t vaddr = vm->start_addr & 0xfffff000;  //页内偏移

	while(size < vm->size){

		pg_try_free(vaddr);

		vaddr += PG_SIZE;
		if(!size && off)
			size += off;
		else size += PG_SIZE;
	}
}


/**
 * 释放进程资源页
 */
void release_prog_pages(struct task_struct *self){
#ifdef DEBUG
	printk("release prog pages start\n");
#endif

	struct list_head *head = self->vm_struct.vm_list;
	struct list_head *walk = head->next;
	struct vm_area *vm = NULL;

	
	//释放数据页
	while(walk != head){

		vm = list_entry(struct vm_area, vm_tag, walk);
		do_vm_area(vm);
		walk = walk->next;
	}

	//栈和堆
	for(int i = 0; i < 2; i++){
		i == 0 ? 
		(vm = &self->vm_struct.vm_stack): 
		(vm = &self->vm_struct.vm_heap);

		do_vm_area(vm);
	}
	

	//释放页表
	uint32_t *pde = self->pg_dir;
	uint32_t size = 768;
	while(size--){
		if(*pde & 0x1)
			pfree(*pde);
		++pde;
	}
	//释放页目录
	pfree(addr_v2p((uint32_t)self->pg_dir));
#ifdef DEBUG
	printk("release prog pages done\n");
#endif
}

/**
 * 过继孩子给init进程
 */
void adopt_children(struct task_struct *self){
	
	struct task_struct *child = NULL;
	struct list_head *head = &curr->children;
	struct list_head *walk = head->next;
	
	while(walk != head){
		child = list_entry(struct task_struct, par_tag, walk);
		list_add(walk, &init_prog->children);
		child->par_pid = 1;
		walk = walk->next;
	}
}

/**
 * 回收自身资源
 */
static void release_prog_resource(struct task_struct *self){

	//这两步顺序不能颠倒
	release_prog_pages(self);
	vm_struct_dtor(&self->vm_struct);
	
	int i = 0;
	while(i < MAXL_OPENS){
		if(self->fd_table[i])
			sys_close(i);
		++i;
	}
}

void sys__exit(int32_t status){
#ifdef DEBUG
	printk("sys_exit\n");
#endif

	if(curr->pid == 1){

		PANIC("init could not exit");
	}

	adopt_children(curr);
	release_prog_resource(curr);
	//唤醒父进程
	if(curr->par->status == TASK_WATTING)
		thread_unblock(curr->par);

	//挂起自己
	thread_block(TASK_HANGING);
}	

void sys_exit(int32_t status){

	//TODO:还有一些其他工作要做
	sys__exit(status);
}

pid_t sys_wait(int32_t *status){

	struct list_head *head = &curr->children;
	struct list_head *walk = NULL;
	struct task_struct *child = NULL;
	while(1){
		
		walk = head->next;
		//空链表，异常
		if(walk == head){
			printk("err no children\n");
			return -1;
		}

		while(walk != head){
			child = list_entry(struct task_struct, par_tag, walk);
			if(child->status == TASK_HANGING)
				break;
			walk = walk->next;
		}
		
		if(walk == head){
		//	printk("no hanging children\n");
			//找不到，阻塞自己
			thread_block(TASK_WATTING);
#ifdef DEBUG
			printk("wake up\n");
#endif
		}else {
#ifdef DEBUG
			printk("get child\n");
#endif
			//找到了
			thread_exit(child, false);
			break;
		}
	}
	return 0;
}

