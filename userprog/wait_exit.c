#include "stdint.h"
#include "thread.h"
#include "debug.h"
#include "fs_sys.h"
#include "process.h"
#include "thread.h"


/**
 * 如果虚拟地址对应页存在，则释放
 */
void pg_try_free(uint32_t vaddr){
	uint32_t *pde = PDE_PTR(vaddr);
	uint32_t *pte = PTE_PTR(vaddr);
	if((*pde & 0x1) && (*pte & 0x1))
		pfree(*pte);
}

/**
 * 释放进程资源页
 */
void release_prog_pages(struct task_struct *self){
	printk("release prog pages start\n");

	struct list_head *head = self->vm_struct.vm_list;
	struct list_head *walk = head;
	struct vm_area *vm = NULL;
	
	//释放数据页
	while(walk != head){
		vm = list_entry(struct vm_area, vm_tag, walk);
		uint32_t size = 0;
		uint32_t vaddr = vm->start_addr;
		while(size < vm->size){
			pg_try_free(vaddr);
			vaddr += PG_SIZE;
			size += PG_SIZE;
		}
		walk = walk->next;
	}

	//释放页表
	uint32_t *pde = self->pg_dir;
	uint32_t size = 1024;
	while(size--){
		if(*pde & 0x1)
			pfree(*pde);
		++pde;
	}
	//释放页目录
	pfree(self->pg_dir);
	printk("release prog pages done\n");
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
	vm_release(&self->vm_struct);
	
	uint8_t i = 3;
	while(i < MAX_FILES_OPEN_PER_PROC){
		if(self->fd_table[i] != -1)
			sys_close(i);
		++i;
	}
}

void sys_exit(int32_t status){
	printk("sys_exit\n");

	adopt_children(curr);
	release_prog_resource(curr);
	//唤醒父进程
	if(curr->par->status == TASK_WATTING)
		thread_unblock(curr->par);

	//挂起自己
	thread_block(TASK_HANGING);
	printk("sys_exit done\n");
}	

pid_t sys_wait(int32_t status){

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
			printk("no hanging children\n");
			//找不到，阻塞自己
			thread_block(TASK_WATTING);
			printk("wake up\n");
		}else {
			printk("get child\n");
			//找到了
			thread_exit(child, false);
			break;
		}
	}
	return 0;
}

