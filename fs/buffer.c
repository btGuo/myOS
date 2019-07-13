#include "buffer.h"
#include "memory.h"
#include "ide.h"
#include "inode.h"
#include "string.h"
#include "fs.h"
#include "block.h"

#define BUFFER_HEAD_SIZE 1024


/**
 * @brief 比较函数，通过函数指针回调，性能上可能有问题
 */
bool compare_inode(struct list_head *elem, uint32_t key){
	struct inode_info *m_inode = list_entry(struct inode_info, hash_tag, elem);
	return m_inode->i_no == key;
}

bool compare_block(struct list_head *elem, uint32_t key){
	struct buffer_head *bh = list_entry(struct buffer_head, hash_tag, elem);
	return bh->blk_nr == key;
}


/**
 * @brief 缓存替换先进先出
 *
 * @return 将被删除的节点
 * 	@retval NULL 缓冲区已经满了
 */
static struct inode_info *i_buffer_pop(struct list_head *head){
	struct list_head *cur = head->next;
	struct inode_info *m_inode = NULL;
	while(cur != head){
		m_inode = list_entry(struct inode_info, queue_tag, cur);
		if(!m_inode->i_lock){
			//从队列中删除
			list_del(cur);
			//从哈希表中删除
			list_del(&m_inode->hash_tag);
			return m_inode;
		}
		cur = cur->next;
	}
	return NULL;
}

static struct buffer_head *b_buffer_pop(struct list_head *head){
	struct list_head *cur = head->next;
	struct buffer_head *bh = NULL;
	while(cur != head){
		bh = list_entry(struct buffer_head, queue_tag, cur);
		if(!bh->lock){
			list_del(cur);
			list_del(&bh->hash_tag);
			return bh;
		}
		cur = cur->next;
	}
	return NULL;
}

/**
 * @brief 同步磁盘
 */
static void buffer_sync_disk(struct disk_buffer *d_buf){
	struct list_head *head = &d_buf->b_queue;
	struct list_head *cur = head->next;
	struct buffer_head *bh = NULL;

	while(cur != head){
		bh = list_entry(struct buffer_head, queue_tag, cur);
		if(bh->dirty){
			//printk("write %d\n", bh->blk_nr);
			//直接写入磁盘
			write_direct(d_buf->part, bh->blk_nr, bh->data, 1);
			//复位脏
			bh->dirty = false;
		}
		cur = cur->next;
	}
}
/**
 * @brief 同步索引节点
 */
static void buffer_sync_inodes(struct disk_buffer *d_buf){
	struct list_head *head = &d_buf->i_queue;
	struct list_head *cur = head->next;
	struct inode_info *m_inode = NULL; 
	struct inode_pos pos;
	struct buffer_head *bh;

	//遍历缓存队列
	while(cur != head){
		m_inode = list_entry(struct inode_info, queue_tag, cur);
		//节点是脏的
		if(m_inode->i_dirty){
			//定位inode
			//printk("write inode %d %d\n", m_inode->i_no, m_inode->i_size);
			inode_locate(d_buf->part, m_inode->i_no, &pos);
			bh = read_block(d_buf->part, pos.blk_nr);
			memcpy((bh->data + pos.off_size), m_inode, sizeof(struct inode));
			write_block(d_buf->part, bh);
			release_block(bh);
			//复位脏
			m_inode->i_dirty = false;
		}
		cur = cur->next;
	}
}

/**
 * @brief 初始化磁盘缓冲区
 */
void disk_buffer_init(struct disk_buffer *d_buf, struct partition *part){

	d_buf->part = part;

	d_buf->b_max_size = BUFFER_HEAD_SIZE;
	d_buf->i_max_size = BUFFER_HEAD_SIZE;

	d_buf->b_size = 0;
	d_buf->i_size = 0;

	LIST_HEAD_INIT(d_buf->b_queue);
	LIST_HEAD_INIT(d_buf->i_queue);

	hash_table_init(&d_buf->b_map, (hash_func)compare_block);
	hash_table_init(&d_buf->i_map, (hash_func)compare_inode);

}

/**
 * @brief 缓存读
 *
 * @param blk_nr 要读取的块号
 *
 * @return 缓冲区头
 * 	@retval NULL 查找为空
 */
struct buffer_head *buffer_read_block(struct disk_buffer *d_buf, uint32_t blk_nr){

	struct list_head *lh = hash_table_find(&d_buf->b_map, blk_nr);
	struct buffer_head *bh = NULL;
	if(lh){
		bh = list_entry(struct buffer_head, hash_tag, lh);
		//上锁
		bh->lock = true;
	       	return bh;	
	}
	return NULL;
}	

struct inode_info *buffer_read_inode(struct disk_buffer *d_buf, uint32_t i_no){
	
	struct list_head *lh = hash_table_find(&d_buf->i_map, i_no);
	struct inode_info *m_inode = NULL;
	if(lh){
		m_inode = list_entry(struct inode_info, hash_tag, lh);
		m_inode->i_lock = true;
		return m_inode;
	}
	return NULL;
}
/**
 * @brief 将块加入缓冲
 *
 * @param bh 缓冲区头
 *
 * @attention 缓冲区有没有上锁取决于调用方，如果没有上锁可能会被换下内存
 *
 * @reture 加入缓冲结果
 * 	@retval true 加入缓冲区成功
 * 	@retval false 加入缓冲区失败
 */
bool buffer_add_block(struct disk_buffer *d_buf, struct buffer_head *bh){
	if(d_buf->b_size == d_buf->b_max_size){
		buffer_sync_disk(d_buf);
		struct buffer_head *old_bh = b_buffer_pop(&d_buf->b_queue);
		if(!old_bh){
			return false;
		}
		kfree(old_bh->data);
		kfree(old_bh);
		--d_buf->b_size;
	}


	//printk("add block %d\n", bh->blk_nr);
	++d_buf->b_size;
	list_add_tail(&bh->queue_tag, &d_buf->b_queue);
	hash_table_insert(&d_buf->b_map, &bh->hash_tag, bh->blk_nr);
	//printk("buffer add block\n");
	return true;
}

bool buffer_add_inode(struct disk_buffer *d_buf, struct inode_info *m_inode){
	if(d_buf->i_size == d_buf->i_max_size){
		buffer_sync_inodes(d_buf);
		struct inode_info *old_inode = i_buffer_pop(&d_buf->i_queue);
		if(!old_inode){
			return false;
		}
		kfree(old_inode);
		--d_buf->i_size;
	}

	++d_buf->i_size;
	list_add_tail(&m_inode->queue_tag, &d_buf->i_queue);
	hash_table_insert(&d_buf->i_map, &m_inode->hash_tag, m_inode->i_no);
	//printk("buffer add inode\n");
	return true;
}

/**
 * 对外接口，同步缓冲区所有内容
 */
void buffer_sync(struct disk_buffer *d_buf){

	//注意顺序
	buffer_sync_inodes(d_buf);
	buffer_sync_disk(d_buf);
}
