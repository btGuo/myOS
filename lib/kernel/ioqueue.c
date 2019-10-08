#include "ioqueue.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"

#define next_pos(x) (((x) + 1) % BUF_SIZE)
/**
 * 初始化队列
 */
void ioqueue_init(struct ioqueue *que){

	mutex_lock_init(&que->lock);
	que->producer = NULL;
	que->consumer = NULL;
	que->head = que->tail = 0;
	que->buf = kmalloc(BUF_SIZE);
	que->size = BUF_SIZE;
}

/**
 * 队列是否空
 */
static bool queue_empty(struct ioqueue *que){
	return que->head == que->tail;
}

/**
 * 队列是否满
 */
static bool queue_full(struct ioqueue *que){
	return next_pos(que->head) == que->tail;
}

/**
 * 从队列中读取
 * @param que 队列
 * @param buf 输出缓冲
 * @param cnt 读取字节数
 * @param type 类型，是否阻塞
 *
 * @return 读取字节数
 */
uint32_t queue_read(struct ioqueue *que, 
		char *buf, 
		uint32_t cnt,
		uint32_t type)
{
	ASSERT(intr_get_status() == INTR_OFF);
	
	if(type == IOQUEUE_BLOCK){
		
		while(queue_empty(que)){
			mutex_lock_acquire(&que->lock);
			que->consumer = curr;
			thread_block(TASK_BLOCKED);
			mutex_lock_release(&que->lock);
		}
	}

	uint32_t res = queue_len(que);

	uint32_t bytes = cnt < res ? cnt : res;
	uint32_t ret = bytes;
	while(bytes--){
		*buf++ = que->buf[que->tail];
		que->tail = next_pos(que->tail);
	}

	if(que->producer != NULL){

		thread_unblock(que->producer);
		que->producer = NULL;
	}

	return ret;
}	

/**
 * 往队列写入
 * @param que 队列
 * @param buf 输入缓冲
 * @param cnt 读取字节数
 * @param type 类型，是否阻塞
 *
 * @return 写入字节数
 */
uint32_t queue_write(struct ioqueue *que,
		const char *buf,
		uint32_t cnt,
		uint32_t type)
{
	ASSERT(intr_get_status() == INTR_OFF);

	if(type == IOQUEUE_BLOCK){

		while(queue_full(que)){
			mutex_lock_acquire(&que->lock);
			que->producer = curr;
			thread_block(TASK_BLOCKED);
			mutex_lock_release(&que->lock);
		}
	}
	
	uint32_t res = que->size - queue_len(que);

	uint32_t bytes = cnt < res ? cnt : res;
	uint32_t ret = bytes;

	while(bytes--){
		que->buf[que->head] = *buf++;
		que->head = next_pos(que->head);
	}

	if(que->consumer != NULL){
		thread_unblock(que->consumer);
		que->consumer = NULL;
	}

	return ret;
}



/**
 * 队列长度
 */
uint32_t queue_len(struct ioqueue *que){

	return que->head >= que->tail ?
		que->head - que->tail :
		que->head + que->size - que->tail;
}

/**
 * 释放队列缓冲区及本身
 */
void queue_release(struct ioqueue *que){
	kfree(que->buf);
	kfree(que);
}

