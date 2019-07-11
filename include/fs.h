#ifndef __FS_FS_H
#define __FS_FS_H

#include "memory.h"
#include "ide.h"
#include "buffer.h"
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

#define ORDER 8
#define LBA_PER_BLK  (BLOCK_SIZE / 4)
#define BLOCK_LEVEL_0 5
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
	((i) == 1 ? BLK_IDX_1(x):\
	(i) == 2 ? BLK_IDX_2(x):\
		 BLK_IDX_3(x)) * 4)

#define BLK_LEVEL(idx)(\
	(idx) < BLOCK_LEVEL_1 ? 1 :\
	(idx) < BLOCK_LEVEL_2 ? 2 : 3)

#define BLK_IDX(idx)(\
	(idx) -= ((idx) < BLOCK_LEVEL_1 ? BLOCK_LEVEL_0 :\
		  (idx) < BLOCK_LEVEL_2 ? BLOCK_LEVEL_1 : BLOCK_LEVEL_2))


#define ALLOC_BH(bh)\
do{\
	bh = (struct buffer_head *)sys_malloc(sizeof(struct buffer_head));\
	if(!bh){\
		PANIC("no more space\n");\
	}\
	bh->data = (uint8_t *)sys_malloc(BLOCK_SIZE);\
	if(!bh->data){\
		PANIC("no more space\n");\
	}\
}while(0)

#define GROUP_INNER(gp, cnt, off)({\
	(gp)->free_blocks_count -= (cnt);\
	GROUP_BLKS - (gp)->free_blocks_count - (cnt) + 1 + \
		(off) * GROUP_BLKS;\
})

/**
 * 文件类型
 */
enum file_types{
	FT_UNKNOWN,  ///< 未知文件
	FT_REGULAR,  ///< 普通文件
	FT_DIRECTORY  ///< 目录
};
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

void filesys_init();

void write_block(struct partition *part, struct buffer_head *bh);
struct buffer_head *read_block(struct partition *part, uint32_t blk_nr);
void release_block(struct buffer_head *bh);
void write_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);
void read_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);
int32_t sys_open(const char *path, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_unlink(const char *path);
int32_t sys_write(int32_t fd, const void *buf, uint32_t count);
void sync();
#endif
