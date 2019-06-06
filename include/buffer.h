#ifndef __LIB_IO_BUFFER_H
#define __LIB_IO_BUFFER_H

#include "list.h"
#include "global.h"
#include "hash_table.h"

struct buffer_head{
	struct list_head hash_tag;
	struct list_head queue_tag;
	uint32_t blk_nr;
	uint8_t *data;
	bool dirty;
};	

/**
 * @brief 磁盘缓冲区描述符
 */
struct disk_buffer{
	struct partition *part;         ///< 分区指针
	uint32_t b_size;                  ///< 缓冲区头个数
	uint32_t i_size;
	struct buffer_head *b_headers;    ///< 缓冲区头
	struct buffer_head *i_headers;
	struct hash_table b_map;          ///< 块映射表
	struct hash_table i_map;        ///< 索引节点映射表
	struct list_head b_queue;        ///< 块缓存队列
	struct list_head i_queue;  ///< 索引节点缓存队列
};

void disk_buffer_init(struct disk_buffer *d_buf);
void buffer_head_init(struct buffer_head *bh, uint32_t blk_nr);
struct buffer_head * get_buffer_head(struct disk_buffer *d_buf);

struct buffer_head *buffer_read(struct disk_buffer *d_buf, uint32_t blk_nr);

void buffer_write(struct disk_buffer *d_buf, struct buffer_head *bh);

struct inode_info *buffer_get_inode(struct disk_buffer *d_buf, uint32_t i_no);
#endif
	uint32_t *pgdir_bak = curr->pg_dir;
	curr->pg_dir = NULL;
	curr->pg_dir = pgdir_bak;
