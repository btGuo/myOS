#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"
#include "ide.h"


typedef long time_t;
#define N_BLOCKS 8 ///< 5个直接，1个间接，1个双间接，1个三间接

/**
 * 磁盘上索引节点结构
 */
struct inode{
	uint16_t i_type;     ///< 文件类型
	uint16_t i_mode;     ///< 访问模式位
	uint16_t i_uid;      ///< 用户id
	uint16_t i_gid;      ///< 用户组id
	uint32_t i_size;     ///< 文件大小字节为单位
	time_t   i_atime;    ///< 访问时间
	time_t   i_ctime;    ///< 创建时间
	time_t   i_mtime;    ///< 修改时间
	time_t   i_dtime;    ///< 删除时间
	uint16_t i_links;    ///< 硬链接数
	uint32_t i_block[N_BLOCKS];  ///< 数据块指针
};	
/**
 * 内存上索引节点结构
 */
struct inode_info{
	uint16_t i_type;     ///< 文件类型
	uint16_t i_mode;     ///< 访问模式位
	uint16_t i_uid;      ///< 用户id
	uint16_t i_gid;      ///< 用户组id
	uint32_t i_size;     ///< 文件大小字节为单位
	time_t   i_atime;    ///< 访问时间
	time_t   i_ctime;    ///< 创建时间
	time_t   i_mtime;    ///< 修改时间
	time_t   i_dtime;    ///< 删除时间
	uint16_t i_links;    ///< 硬链接数
	uint32_t i_block[N_BLOCKS];  ///< 数据块指针
	/** 以下为内存特有 */
	uint16_t i_blocks;   ///< 文件大小块为单位
	uint32_t i_no;       ///< 索引节点号
	uint32_t i_open_cnts;///< 打开次数 
	bool     i_dirty;    ///< 脏标志
	bool     i_lock;     ///< 上锁意味着必须留在内存
	bool     i_buffered; ///< 是否位于缓冲区
	struct list_head hash_tag;
	struct list_head queue_tag;
};

struct inode_pos{
	uint32_t blk_nr;
	uint32_t off_size;
};

void inode_locate(struct partition *part, uint32_t i_no, struct inode_pos *pos);
struct inode_info *inode_open(struct partition *part, uint32_t i_no);
void inode_close(struct inode_info *inode);
void inode_init(struct inode_info *m_inode, uint32_t i_no);
void release_inode(struct inode_info *m_inode);
void write_inode(struct inode_info *m_inode);

struct inode_info *buffer_read_inode(struct disk_buffer *d_buf, uint32_t i_no);
void buffer_add_inode(struct disk_buffer *d_buf, struct inode_info *m_inode);
void buffer_release_inode(struct inode_info *m_inode);
#endif
