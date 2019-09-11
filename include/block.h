#ifndef FS_BLOCK_H
#define FS_BLOCK_H


#include "inode.h"
#include "buffer.h"
#include "fs.h"


/** 用于get_block_num mode指定模式 */
#define M_SEARCH 0  ///< 只查找
#define M_CREATE 1  ///< 创建块

int32_t block_bmp_alloc(struct fext_fs *fs);
void block_bmp_clear(struct fext_fs *fs, uint32_t blk_nr);
void write_block(struct fext_fs *fs, struct buffer_head *bh);
struct buffer_head *read_block(struct fext_fs *fs, uint32_t blk_nr);
void release_block(struct buffer_head *bh);
void clear_blocks(struct fext_inode_m *m_inode);
uint32_t get_block_num(struct fext_inode_m *inode, uint32_t idx, uint8_t mode);
void remove_last(struct fext_inode_m *m_inode);
void write_direct(struct fext_fs *fs, uint32_t sta_blk_nr, void *data, uint32_t cnt);
void read_direct(struct fext_fs *fs, uint32_t sta_blk_nr, void *data, uint32_t cnt);

#endif
