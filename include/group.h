#ifndef __FS_GROUP_H
#define __FS_GROUP_H

#include "stdint.h"

#define GROUP_PTR(gp, i_no)\
	((gp) + ((i_no) / INODES_PER_GROUP))

/**
 * 组描述符
 */
struct group{
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
 * @brief 内存上组描述符
 */
struct group_info{
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
	struct group_info *next;       ///< 下一个组指针
	uint32_t group_nr;             ///< 组号
};

void group_info_init(struct partition *part, struct group_info *gp){
	if(!gp->block_bmp->bits){
		struct buffer_head *bh = read_block(part, gp->block_bitmap);
		gp->block_bmp->bits = bh->data;
		gp->block_bmp->byte_len = BLOCKS_PER_GROUP / 8;
	}
	if(!gp->inode_bmp->bits){
		struct buffer_head *bh = read_block(part, gp->inode_bitmap);
		gp->inode_bmp->bits = bh->data;
		gp->inode_bmp->byte_len = INODES_PER_GROUP / 8;
	}
}


#endif
