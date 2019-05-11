#ifndef __THREAD_SYNC_H
#define __THREAD_SYNC_H
#include "list.h"
#include "stdint.h"
#include "thread.h"
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
	struct task_struct *holder;
	uint8_t value;
};

#endif


