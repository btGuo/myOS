#ifndef __FS_GROUP_H
#define __FS_GROUP_H

#include <stdint.h>
#include "bitmap.h"
#include "fs.h"
#include "super_block.h"

/**
 * 组描述符
 */
struct fext_group{
	uint32_t block_bitmap;         ///< 数据块位图块号
	uint32_t inode_bitmap;         ///< 索引节点位图块号
	uint32_t inode_table;          ///< 索引节点表块号
	uint16_t free_blocks_count;    ///< 空闲数据块个数
	uint16_t free_inodes_count;    ///< 空闲索引节点个数
	uint16_t used_dirs_count;      ///< 目录个数
	uint16_t pad;                  ///< 对齐用
	uint32_t zeros[3];             ///< 补全32字节
};

/**
 * 内存上组描述符
 */
struct fext_group_m{
	uint32_t block_bitmap;         ///< 数据块位图块号
	uint32_t inode_bitmap;         ///< 索引节点位图块号
	uint32_t inode_table;          ///< 索引节点表块号
	uint16_t free_blocks_count;    ///< 空闲数据块个数
	uint16_t free_inodes_count;    ///< 空闲索引节点个数
	uint16_t used_dirs_count;      ///< 目录个数
	uint16_t pad;                  ///< 对齐用
	uint32_t zeros[3];             ///< 补全32字节
	struct bitmap inode_bmp;       ///< 索引节点位图
	struct bitmap block_bmp;       ///< 块节点位图
	struct buffer_head *inode_bmp_bh;  ///< 当前inode位图缓冲区指针
	struct buffer_head *block_bmp_bh;  ///< 当前block位图缓冲区指针
	uint32_t group_nr;             ///< 组号
};

static inline struct fext_group_m * 
fext_group_ptr(struct fext_fs *fs, uint32_t i_no)
{
	return (fs->groups + (i_no / fs->sb->inodes_per_group));
}

void group_info_init(struct fext_fs *fs, struct fext_group_m *gp);
struct fext_group_m *group_switch(struct fext_fs *fs);
void group_bmp_sync(struct fext_fs *fs);


#endif
