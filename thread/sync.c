#include "sync.h"
#include "interrupt.h"
#include "debug.h"
#include "thread.h"
extern struct list_head thread_ready_list;
extern struct task_struct *curr;

void sema_init(struct semaphore *sema, uint8_t value){
	sema->value = value;
	LIST_HEAD_INIT(sema->waiters);
}

void sema_down(struct semaphore *sema){
	enum intr_status old_stat = intr_disable();
	while(sema->value == 0){
		//先删除后加入, 顺序很重要
		list_del(&curr->ready_tag);
		list_add_tail(&curr->ready_tag, &sema->waiters);
		thread_block(TASK_BLOCKED);
	}
	--sema->value;
	intr_set_status(old_stat);
}

void sema_up(struct semaphore *sema){
	enum intr_status old_stat = intr_disable();
	if(!list_empty(&sema->waiters)){
		struct task_struct *nxt = list_entry(struct task_struct, ready_tag, sema->waiters.next);
		list_del(sema->waiters.next);
    	thread_unblock(nxt);
	}
	++sema->value;
	intr_set_status(old_stat);
}	


void mutex_lock_init(struct mutex_lock *lock){
	lock->holder = NULL;
	sema_init(&lock->semaphore, 1);
	lock->holder_repeat_nr = 0;
}

void mutex_lock_acquire(struct mutex_lock *lock){
	if(lock->holder != curr){
		lock->holder = curr;
		sema_down(&lock->semaphore);
	}
	++lock->holder_repeat_nr;
}

void mutex_lock_release(struct mutex_lock *lock){
	if(lock->holder_repeat_nr > 1){
		--lock->holder_repeat_nr;
		return;
	}
	lock->holder = NULL;
	lock->holder_repeat_nr = 0;
	sema_up(&lock->semaphore);
}

void spin_lock_init(struct spin_lock *lock){
	lock->value = 0;
}

void spin_lock_acquire(struct spin_lock *lock){
	while(lock->value);
	lock->value = 1;
}

void spin_lock_release(struct spin_lock *lock){
	lock->value = 0;
}
