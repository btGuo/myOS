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
 * 从队列中换取字符
 */
char queue_getchar(struct ioqueue *que){
	ASSERT(intr_get_status() == INTR_OFF);

	while(queue_empty(que)){
		mutex_lock_acquire(&que->lock);
		que->consumer = curr;
		thread_block(curr);
		mutex_lock_release(&que->lock);
	}

	char ch = que->buf[que->tail];
	que->tail = next_pos(que->tail);

	if(que->producer != NULL){
		thread_unblock(que->producer);
		que->producer = NULL;
	}
	return ch;
}

/**
 * 向队列中放字符
 */
void queue_putchar(struct ioqueue *que, char ch){
	ASSERT(intr_get_status() == INTR_OFF);
	
	//这里得用while
	while(queue_full(que)){
		mutex_lock_acquire(&que->lock);
		que->producer = curr;
		thread_block(curr);
		mutex_lock_release(&que->lock);
	}
	que->buf[que->head] = ch;
	que->head = next_pos(que->head);
	if(que->consumer != NULL){
		thread_unblock(que->consumer);
		que->consumer = NULL;
	}
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

