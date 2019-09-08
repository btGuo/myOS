#ifndef FS_BLOCK_H
#define FS_BLOCK_H


#include "inode.h"
#include "buffer.h"
#include "fs.h"


/** 用于get_block_num mode指定模式 */
#define M_SEARCH 0  ///< 只查找
#define M_CREATE 1  ///< 创建块

#define ORDER 8    ///< 对应LBA_PER_BLK 位数为8
#define LBA_PER_BLK  (BLOCK_SIZE / 4) ///< 一块可以存放多少lba地址，每个lba地址32位，4字节

#define BLOCK_LEVEL_0 5         ///< 直接块5
#define BLOCK_LEVEL_1 (BLOCK_LEVEL_0 + LBA_PER_BLK)  ///< 一次间接
#define BLOCK_LEVEL_2 (BLOCK_LEVEL_1 + LBA_PER_BLK * LBA_PER_BLK)   ///< 二次间接
#define BLOCK_LEVEL_3 (BLOCK_LEVEL_2 + LBA_PER_BLK * LBA_PER_BLK * LBA_PER_BLK)  ///< 三次间接

int32_t block_bmp_alloc(struct fext_fs *fs);
void block_bmp_clear(struct fext_fs *fs, uint32_t blk_nr);
void write_block(struct fext_fs *fs, struct buffer_head *bh);
struct buffer_head *read_block(struct fext_fs *fs, uint32_t blk_nr);
void release_block(struct buffer_head *bh);
void clear_blocks(struct fext_inode_m *m_inode);
uint32_t get_block_num(struct fext_inode_m *inode, uint32_t idx, uint8_t mode);
void remove_last(struct fext_inode_m *m_inode);

#endif
