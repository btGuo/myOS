#ifndef __LIB_IO_BUFFER_H
#define __LIB_IO_BUFFER_H

#include "global.h"
#include "hash_table.h"
#include "stdint.h"
#include "list.h"

/**
 * @brief 磁盘块头信息，缓冲用，对应一个磁盘块
 */
struct buffer_head{
	struct list_head hash_tag;   ///< 哈希表标签
	struct list_head queue_tag;  ///< 队列标签
	uint32_t blk_nr;             ///< 块号
	uint8_t *data;               ///< 数据指针
	bool dirty;                  ///< 脏标志位
	bool lock;                   ///< 锁标志
	bool is_buffered;            ///< 是否在缓存中
};	

/**
 * @brief 磁盘缓冲区描述符
 */
struct disk_buffer{
	struct partition *part;          ///< 分区指针
	uint32_t b_max_size;             ///< 磁盘块最大缓冲数
	uint32_t b_size;                 ///< 磁盘块当前缓冲数
	uint32_t i_max_size;             ///< 索引节点最大缓冲数
	uint32_t i_size;                 ///< 索引节点当前缓冲数
	struct hash_table b_map;         ///< 块映射表
	struct hash_table i_map;         ///< 索引节点映射表
	struct list_head b_queue;        ///< 块缓存队列
	struct list_head i_queue;        ///< 索引节点缓存队列
};

#define BUFW_BLOCK(bh) ((bh)->dirty = true)
#define BUFR_BLOCK(bh) ((bh)->lock = false)
#define BUFW_INODE(m_inode) ((m_inode)->i_dirty = true)
//#define BUFR_INODE(m_inode) ((m_inode)->i_lock = true)

void disk_buffer_init(struct disk_buffer *d_buf);

struct buffer_head *buffer_read_block(struct disk_buffer *d_buf, uint32_t blk_nr);
bool buffer_add_block(struct disk_buffer *d_buf, struct buffer_head *bh);
#endif
