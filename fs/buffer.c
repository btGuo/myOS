#include "ide.h"
#include "buffer.h"
#include "memory.h"

#define BUFFER_HEAD_SIZE 1024
/**
 * @brief 初始化磁盘缓冲区
 */
void disk_buffer_init(struct disk_buffer *d_buf){
	d_buf->size = BUFFER_HEAD_SIZE;
	uint32_t bytes = sizeof(struct buffer_head) * d_buf->size;
	d_buf->headers = sys_malloc(bytes);
	memset(d_buf->headers, bytes);
	LIST_HEAD_INIT(d_buf->queue);
	LIST_HEAD_INIT(d_buf->open_inodes);
	hash_table_init(&d_buf->map);
	d_buf->page = NULL;
	d_buf->page_off = 0;
}

/**
 * @brief 初始化缓冲区头，分配内存(如果内存还没分配), 
 * 设置块号，加入队列及哈希表，将脏位设置为假
 */
void buffer_head_init(struct buffer_head *bh, uint32_t blk_nr){
	if(!bh->data)
		bh->data = (uint8_t *)sys_malloc(BLOCK_SIZE);
	bh->blk_nr = blk_nr;
	bh->dirty = false;
	list_add_tail(&bh->queue_tag, &d_buf->queue);
	hash_table_insert(&d_buf->map, &bh->hash_tag);
}

/**
 * @brief 找到一个空的缓冲区头
 */
struct buffer_head *get_buffer_head(struct disk_buffer *d_buf){

	uint32_t i = 0;
	while(i < d_buf->size){
		if(d_buf->headers[i].blk_nr == 0 || !d_buf->headers[i].dirty){
			return &d_buf->headers[i];
		}
		++i;
	}
	sync_disk(d_buf);
	return fifo(d_buf);
}

/**
 * @brief 缓存读
 */
struct buffer_head *buffer_read(struct disk_buffer *d_buf, uint32_t blk_nr){
	struct buffer_read *bh = hash_table_find(&d_buf->map, blk_nr);
	if(bh) 
		return bh;
	bh = get_buffer_head(d_buf);
	buffer_head_init(bh, blk_nr);
	ide_read(d_buf->part->disk, bh->blk_nr, bh->data, 1);
	return bh;
}	

/**
 * @brief 缓存写
 */
void buffer_write(struct disk_buffer *d_buf, struct buffer_head *bh){
	bh->dirty = true;
}

/**
 * @brief 缓存替换先进先出
 */
static struct buffer_head *fifo(struct disk_buffer *d_buf){
	struct list_head *elem = &d_buf->queue->next;
	struct buffer_head *bh = list_entry(struct buffer_head, hash_tag, elem);
	list_del(elem);
	list_del(&bh->hash_tag);
	return bh;
}

/**
 * @brief 同步磁盘
 */
void sync_disk(struct disk_buffer *d_buf){
	uint32_t i = 0;
	struct buffer_head *bhs = d_buf->headers;

	while(i < d_buf->size){
		if(bhs[i].blk_nr != 0 && bhs[i].dirty){
			ide_write(d_buf->part->disk, bhs[i].blk_nr, bhs[i].data, 1);
			bhs[i].dirty = false;
			++i;
		}
	}
}
/**
 * @brief 查找缓冲中索引节点
 *
 * @return 索引节点
 * 	@retval NULL 查找为空
 */
struct inode_info *buffer_get_inode(struct disk_buffer *d_buf, uint32_t i_no){
	
	struct inode_info *m_inode = NULL;
	uint32_t len = list_len(&part->open_inodes);
	struct list_head *elem = &part->open_inodes;
	while(len--){
		elem = elem->next;
		m_inode = list_entry(struct inode, inode_tag, elem);
		if(m_inode->i_no == i_no){
			m_inode->i_open_cnts++;
			return m_inode;
		}
	}

	uint32_t *pgdir_bak = curr->pg_dir;
	curr->pg_dir = NULL;
	m_inode = (struct inode_info *)sys_malloc(;
	curr->pg_dir = pddir_bak;

	d_buf->page_off += sizeof(struct inode_info);	
	list_add(&inode->i_tag, &d_buf->open_inodes);

	struct inode_pos pos;
	inode_locate(part, i_no, &pos);
	struct buffer_head *bh = read_block(part, pos.blk_nr);
	struct inode *d_inode = bh->data + pos->off_size;
	inode_info_init(m_inode, d_inode, i_no);
	return inode;
}

