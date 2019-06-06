#ifndef __FS_FS_H
#define __FS_FS_H

#define SECTOR_SIZE 512
#define BLOCK_SIZE 1024
#define GROUP_SIZE 8192
#define GROUP_BLKS 8

///每个块有多少位  1024 * 8
#define BITS_PER_BLOCK 8192
#define INODES_PER_GROUP BITS_PER_BLOCK
#define BLOCKS_PER_GROUP BITS_PER_BLOCK

//每个块有多少个扇区
#define BLK_PER_SEC (BLOCK_SIZE / SECTOR_SIZE)

#define BLOCKS_BMP_BLKS 1
#define INODES_BMP_BLKS 1
#define INODES_BLKS(sb) ((sb)->
enum file_types{
	FT_UNKNOWN,
	FT_REGULAR,
	FT_DIRECTORY
};

void filesys_init();

inline void write_block(struct parttition *part, struct buffer_head *bh);
inline struct buffer_head *read_block(struct parttition *part, uint32_t blk_nr);
inline void write_indirect(struct parttition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);
inline void read_indirect(struct partition *part, uint32_t sta_blk_nr, void *data, uint32_t cnt);
#endif
