#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"
#include "fs.h"


typedef long time_t

#define N_BLOCKS 8 ///< 5个直接，1个间接，1个双间接，1个三间接

#define ORDER 8
#define LBA_PER_BLK  (BLOCK_SIZE / 4);
#define BLOCK_LEVEL_0 12
#define BLOCK_LEVEL_1 (BLOCK_LEVEL_0 + LBA_PER_BLK) 
#define BLOCK_LEVEL_2 (BLOCK_LEVEL_1 + LBA_PER_BLK * LBA_PER_BLK)
#define BLOCK_LEVEL_3 (BLOCK_LEVEL_2 + LBA_PER_BLK * LBA_PER_BLK * LBA_PER_BLK)
//对应block_size 大小
#define BLOCK_MASK_1 ((1 << ORDER) - 1)
#define BLOCK_MASK_2 (BLOCK_MASK_1 << ORDER)
#define BLOCK_MASK_3 (BLOCK_MASK_2 << ORDER)

#define BLK_IDX_1(x) ((x) & BLOCK_MASK_1)
#define BLK_IDX_2(x) (((x) & BLOCK_MASK_2) >> ORDER)
#define BLK_IDX_3(x) (((x) & BLOCK_MASK_3) >> (ORDER << 1))

#define BLK_IDX_I(x, i)(\
	(i) == 1 ? BLK_IDX_1(x):\
	(i) == 2 ? BLK_IDX_2(x):\
		 BLK_IDX_3(x))

#define BLK_LEVEL(idx)(\
	(idx) < BLOCK_LEVEL_1 ? 1 :\
	(idx) < BLOCK_LEVEL_2 ? 2 : 3)

#define BLK_IDX(idx)(\
	(idx) -= ((idx) < BLOCK_LEVEL_1 ? BLOCK_LEVEL_0 :\
		  (idx) < BLOCK_LEVEL_2 ? BLOCK_LEVEL_1 : BLOCK_LEVEL_2))

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
	uint16_t i_blocks;   ///< 文件大小块为单位
	struct group *gp;   ///< 块组指针
	uint32_t i_no;      ///< 索引节点号
	struct list_head i_tag;
	uint32_t i_open_cnts;
};

void inode_init(uint32_t i_no, struct inode *inode);
void inode_close(struct inode *inode);
struct inode * inode_open(struct partition *part, uint32_t i_no);
void inode_sync(struct partition *part, struct inode *inode, void *io_buf);
#endif
