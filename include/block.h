#ifndef FS_BLOCK_H
#define FS_BLOCK_H

#include "ide.h"
#include "inode.h"
#include "buffer.h"
#include "fs.h"

#define M_SEARCH 0
#define M_CREATE 1

//下面这组宏名称有点混乱，个人页不知道要起什么名字

#define ORDER 8    ///< 对应LBA_PER_BLK 位数为8
#define LBA_PER_BLK  (BLOCK_SIZE / 4) ///< 一块可以存放多少lba地址，每个lba地址32位，4字节
#define BLOCK_LEVEL_0 5         ///< 直接块5
#define BLOCK_LEVEL_1 (BLOCK_LEVEL_0 + LBA_PER_BLK)  ///< 一次间接
#define BLOCK_LEVEL_2 (BLOCK_LEVEL_1 + LBA_PER_BLK * LBA_PER_BLK)   ///< 二次间接
#define BLOCK_LEVEL_3 (BLOCK_LEVEL_2 + LBA_PER_BLK * LBA_PER_BLK * LBA_PER_BLK)  ///< 三次间接

//对应block_size 大小
#define BLOCK_MASK_1 ((1 << ORDER) - 1)
#define BLOCK_MASK_2 (BLOCK_MASK_1 << ORDER)
#define BLOCK_MASK_3 (BLOCK_MASK_2 << ORDER)

#define BLK_IDX_1(x) ((x) & BLOCK_MASK_1)
#define BLK_IDX_2(x) (((x) & BLOCK_MASK_2) >> ORDER)
#define BLK_IDX_3(x) (((x) & BLOCK_MASK_3) >> (ORDER << 1))


static inline uint32_t BLK_IDX_I(uint32_t x, uint32_t i){
	return (i == 1 ? BLK_IDX_1(x):
		i == 2 ? BLK_IDX_2(x):
		BLK_IDX_3(x)) * 4;
}

static inline uint32_t BLK_LEVEL(uint32_t idx){
	return idx < BLOCK_LEVEL_1 ? 1 :
		idx < BLOCK_LEVEL_2 ? 2 : 3;
}

static inline uint32_t BLK_IDX(uint32_t idx){
	return idx - (idx < BLOCK_LEVEL_1 ? BLOCK_LEVEL_0 :
			idx < BLOCK_LEVEL_2 ? BLOCK_LEVEL_1 :
			BLOCK_LEVEL_2);
}


void write_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);
void read_direct(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);

void write_block(struct partition *part, struct buffer_head *bh);
struct buffer_head *read_block(struct partition *part, uint32_t blk_nr);

void release_block(struct buffer_head *bh);
void clear_blocks(struct partition *part, struct inode_info *m_inode);
void init_block(struct partition *part, uint32_t blk_nr);

int32_t block_bmp_alloc(struct partition *part);
void block_bmp_clear(struct partition *part, uint32_t blk_nr);

uint32_t get_block_num(struct partition *part, struct inode_info *inode,\
	       	uint32_t idx, uint8_t mode);
void remove_last(struct partition *part, struct inode_info *m_inode);
#endif
