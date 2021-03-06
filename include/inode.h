#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"
#include "fs.h"
#include <sys/types.h>


#define N_BLOCKS 8 ///< 5个直接，1个间接，1个双间接，1个三间接

/**
 * 磁盘上索引节点结构，64字节
 */
struct fext_inode{
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
struct fext_inode_m{
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
	bool     i_write_deny;   ///< 拒绝写标志
	bool     i_mounted;       ///< 是否挂载点
	struct list_head hash_tag;   ///< 哈希表标签
	struct list_head queue_tag;  ///< 队列标签
	struct fext_fs *fs;         ///< 文件系统指针
};

/**
 * inode 在块内的位置
 */
struct fext_inode_pos{
	uint32_t blk_nr;    ///< 块号
	uint32_t off_size;  ///< 块内偏移
};



int32_t inode_bmp_alloc(struct fext_fs *fs);
void inode_bmp_clear(struct fext_fs *fs, uint32_t i_no);
void inode_locate(struct fext_fs *fs, uint32_t i_no, struct fext_inode_pos *pos);
void inode_release(struct fext_inode_m *m_inode);
void inode_sync(struct fext_inode_m *m_inode);
struct fext_inode_m *inode_open(struct fext_fs *fs, uint32_t i_no);
struct fext_inode_m *path2inode(const char *path);
void inode_close(struct fext_inode_m *m_inode);
struct fext_inode_m *inode_alloc(struct fext_fs *fs);
void inode_init(struct fext_fs *fs, struct fext_inode_m *m_inode, uint32_t i_no);
bool inode_is_empty(struct fext_inode_m *inode);
void inode_delete(struct fext_fs *fs, uint32_t i_no);

bool buffer_add_inode(struct disk_buffer *d_buf, struct fext_inode_m *m_inode);
struct fext_inode_m *buffer_read_inode(struct disk_buffer *d_buf, uint32_t i_no);
#endif
