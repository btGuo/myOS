#include "buffer.h"
#include "memory.h"
#include "ide.h"
#include "inode.h"

#define BUFFER_HEAD_SIZE 1024


//比较函数，通过函数指针回调，性能上可能有问题
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
 */
static struct list_head *queue_pop(struct list_head *head){
	struct list_head *elem = head->next;
	list_del(elem);
	return elem; 
}

/**
 * @brief 缓存写
 */
void buffer_write_block(struct disk_buffer *d_buf, struct buffer_head *bh){
	bh->dirty = true;
}

void buffer_release_inode(struct inode_info *m_inode){
	m_inode->i_dirty = true;
	m_inode->i_lock = false;
}

/**
 * @brief 同步磁盘
 */
static void sync_disk(struct disk_buffer *d_buf){
	struct list_head *head = &d_buf->b_queue;
	struct list_head *cur = head->next;
	struct buffer_head *bh = NULL;

	while(cur != head){
		bh = list_entry(struct buffer_head, queue_tag, cur);
		if(bh->dirty && !bh->lock){
			ide_write(d_buf->part->disk, bh->blk_nr, bh->data, 1);
			bh->dirty = false;
		}
		cur = cur->next;
	}
}
/**
 * @brief 同步索引节点
 */
static void sync_inodes(struct disk_buffer *d_buf){
	struct list_head *head = &d_buf->i_queue;
	struct list_head *cur = head->next;
	struct inode_info *m_inode = NULL; 
	struct inode_pos pos;
	struct buffer_head *bh;

	//遍历缓存队列
	while(cur != head){
		m_inode = list_entry(struct inode_info, queue_tag, cur);
		if(m_inode->i_dirty && !m_inode->i_lock){
			inode_locate(d_buf->part, m_inode->i_no, &pos);
			bh = buffer_read_block(d_buf, pos.blk_nr);
			memcpy((bh->data + pos.off_size), &m_inode, sizeof(struct inode));
			buffer_write_block(d_buf, bh);
			m_inode->i_dirty = false;
		}
		cur = cur->next;
	}
}

/**
 * @brief 初始化磁盘缓冲区
 */
void disk_buffer_init(struct disk_buffer *d_buf){

	d_buf->b_max_size = BUFFER_HEAD_SIZE;
	d_buf->i_max_size = BUFFER_HEAD_SIZE;

	d_buf->b_size = 0;
	d_buf->i_size = 0;

	LIST_HEAD_INIT(d_buf->b_queue);
	LIST_HEAD_INIT(d_buf->i_queue);

	hash_table_init(&d_buf->b_map, compare_block);
	hash_table_init(&d_buf->i_map, compare_inode);

}

/**
 * @brief 缓存读
 */
struct buffer_head *buffer_read_block(struct disk_buffer *d_buf, uint32_t blk_nr){
	struct list_head *lh = hash_table_find(&d_buf->b_map, blk_nr);
	struct buffer_head *bh = NULL;
	if(lh){
		bh = list_entry(struct buffer_head, hash_tag, lh);
	       	return bh;	
	}
	return NULL;
}	


struct inode_info *buffer_read_inode(struct disk_buffer *d_buf, uint32_t i_no){
	
	struct list_head *lh = hash_table_find(&d_buf->i_map, i_no);
	struct inode_info *m_inode = NULL;
	if(lh){
		m_inode = list_entry(struct inode_info, hash_tag, lh);
		return m_inode;
	}
	return NULL;
}

void buffer_add_block(struct disk_buffer *d_buf, struct buffer_head *bh){
	if(d_buf->b_size == d_buf->b_max_size){
		sync_disk(d_buf);
		struct buffer_head *old_bh = list_entry(struct buffer_head,\
			       	queue_tag, queue_pop(&d_buf->b_queue));
		sys_free(old_bh->data);
		sys_free(old_bh);
		--d_buf->b_size;
	}

	++d_buf->b_size;
	list_add_tail(&bh->queue_tag, &d_buf->b_queue);
	hash_table_insert(&d_buf->b_map, &bh->hash_tag);
}

void buffer_add_inode(struct disk_buffer *d_buf, struct inode_info *m_inode){
	if(d_buf->i_size == d_buf->i_max_size){
		sync_inodes(d_buf);
		struct inode_info *old_inode \
			= list_entry(struct inode_info, queue_tag, queue_pop(&d_buf->i_queue));
		sys_free(old_inode);
		--d_buf->i_size;
	}

	++d_buf->i_size;
	list_add_tail(&m_inode->queue_tag, &d_buf->i_queue);
	hash_table_insert(&d_buf->i_map, &m_inode->hash_tag);
}
