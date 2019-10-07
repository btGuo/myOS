#ifndef __DEVICE_IOQUEUE_H
#define __DEVICE_IOQUEUE_H
#include "sync.h"
#include "stdint.h"

#define BUF_SIZE 4096

#define IOQUEUE_BLOCK 0
#define IOQUEUE_NON_BLOCK 1

/**
 * io环形队列，生产者消费者模型
 */
struct ioqueue{
	struct mutex_lock lock;   ///< 同步锁
	struct task_struct *producer;
	struct task_struct *consumer;
	char *buf;   ///< 数据区
	int32_t head;
	int32_t tail;
	int32_t size;
};

void ioqueue_init(struct ioqueue *que);
uint32_t queue_len(struct ioqueue *que);
void queue_release(struct ioqueue *que);

uint32_t queue_write(struct ioqueue *que, const char *buf, uint32_t cnt, uint32_t type);
uint32_t queue_read(struct ioqueue *que,  char       *buf, uint32_t cnt, uint32_t type);
#endif
