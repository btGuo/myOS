#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"
#include "global.h"

struct semaphore{
	uint8_t value;
	struct list_head waiters;
};

struct mutex_lock{
	struct task_struct *holder;
	struct semaphore semaphore;
	uint32_t holder_repeat_nr;
};


struct spin_lock{
	uint8_t value;
};

void sema_init(struct semaphore *sema, uint8_t value);

void mutex_lock_acquire(struct mutex_lock *lock);
void mutex_lock_release(struct mutex_lock *lock);
void mutex_lock_init(struct mutex_lock *lock);

void spin_lock_acquire(struct spin_lock *lock);
void spin_lock_release(struct spin_lock *lock);
void spin_lock_init(struct spin_lock *lock);
#endif


