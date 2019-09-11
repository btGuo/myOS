#ifndef __FS_FS_H
#define __FS_FS_H

#include "buffer.h"
#include "local.h"
#include "rw_img.h"

#define SECTOR_SIZE 512

/**  引导块大小，固定值  */
#define BOOT_BLKSZ 1024

/**  引导块扇区数        */
#define BOOT_SECS (BOOT_BLKSZ / SECTOR_SIZE)

#define BLOCKS_BMP_BLKS 1
#define INODES_BMP_BLKS 1
#define INODES_BLKS (INODES_PER_GROUP * INODE_SIZE / BLOCK_SIZE)

#define ROOT_INODE 0

#define GROUP_INNER(gp, cnt, off, group_blks)({\
	(gp)->free_blocks_count -= (cnt);\
	group_blks - (gp)->free_blocks_count - (cnt) + 1 + \
		(off) * group_blks;\
})


/************************************************************************************
 *
 * total 
 * --------------------------------------------------------------------
 * |            |               |               |     |               | 
 * | boot block | block group 0 | block group 1 | ... | block group n |
 * |            |               |               |     |               |
 * --------------------------------------------------------------------
 * 
 * for each block group     (group description table -> gdt) 
 * -------------------------------------------------------------------------------
 * |             |     |              |              |             |             |
 * | super block | gdt | block bitmap | inode bitmap | inode table | data blocks |
 * |             |     |              |              |             |             |
 * -------------------------------------------------------------------------------
 *
 * ***********************************************************************************/

/**
 * 文件系统描述符，类fext
 */
struct fext_fs{
	struct fext_super_block *sb;    ///< 分区超级块
	struct fext_group_m *groups;  ///< 块组指针
	struct fext_group_m *cur_gp;  ///< 当前使用块组
	uint32_t groups_cnt;       ///< 块组数
	uint32_t groups_blks;      ///< 块组struct group所占块数
	struct disk_buffer io_buffer;     ///< 磁盘缓冲区，换成指针或许更好
	struct partition *part;    ///< 挂载分区
	struct fext_inode_m *root_i;  ///< 根i节点
	bool mounted;             ///< 是否已挂载
	char mount_path[32]; ///< 挂载路径
	uint32_t order;      ///< 处理块号时用到
	uint32_t direct_blknr;     ///< 直接块数量
	uint32_t s_indirect_blknr;      ///< 一次间接块数量
	uint32_t d_indirect_blknr;      ///< 二次间接块数量
	uint32_t t_indirect_blknr;      ///< 三次间接块数量
	uint32_t max_blocks;            ///< 最大块数
	uint32_t lba_per_blk;           ///< 每块可以存放多少块号
	uint32_t sec_per_blk;           ///< 每块有多少扇区
};

extern struct fext_fs *root_fs;  ///< 根文件系统

void disk_buffer_init(struct disk_buffer *d_buf, struct fext_fs *fs);
void init_fs(struct partition *part);
#endif
