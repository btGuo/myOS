#ifndef __FS_FS_H
#define __FS_FS_H

#include "buffer.h"
#include <sys/types.h>
#include <sys/stat.h>

#define SECTOR_SIZE 512
#define BLOCK_SIZE 1024
#define GROUP_SIZE (8192 * BLOCK_SIZE)
#define GROUP_BLKS 8192

///每个块有多少位  1024 * 8
#define BITS_PER_BLOCK 8192
#define INODES_PER_GROUP BITS_PER_BLOCK
#define BLOCKS_PER_GROUP BITS_PER_BLOCK

//每个块有多少个扇区
#define BLK_PER_SEC (BLOCK_SIZE / SECTOR_SIZE)
//引导块数
#define LEADER_BLKS 1

#define INODE_SIZE 64

#define BLOCKS_BMP_BLKS 1
#define INODES_BMP_BLKS 1
#define INODES_BLKS (INODES_PER_GROUP * INODE_SIZE / BLOCK_SIZE)

#define ROOT_INODE 0

#define MAX_BLOCK_DIR_POS 4

#define GROUP_INNER(gp, cnt, off)({\
	(gp)->free_blocks_count -= (cnt);\
	GROUP_BLKS - (gp)->free_blocks_count - (cnt) + 1 + \
		(off) * GROUP_BLKS;\
})

/**
 * 文件打开标志位
 */
enum oflags{
	O_RDONLY,    ///< 只读  000b
	O_WRONLY,    ///< 只写  001b
	O_RDWR,      ///< 读写  010b
	O_CREAT = 4  ///< 创建  100b
};
/**
 * 文件读写指针标志
 */
enum whence{
	SEEK_SET = 1,  ///< 以文件开始处为参照
	SEEK_CUR,      ///< 以文件当前位置为参照
	SEEK_END       ///< 以文件末尾为参照
};

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
};

extern struct fext_fs *root_fs;  ///< 根文件系统

void sync();
void filesys_init();
void disk_buffer_init(struct disk_buffer *d_buf, struct fext_fs *fs);
#endif
