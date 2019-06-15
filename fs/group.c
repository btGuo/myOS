#include "group.h"
#include "buffer.h"
#include "fs.h"
#include "ide.h"
/**
 * 初始化组描述符，如果位图不在内存中则加载到内存并初始化
 */
void group_info_init(struct partition *part, struct group_info *gp){
	if(!(gp->block_bmp).bits){
		struct buffer_head *bh = read_block(part, gp->block_bitmap);
		gp->block_bmp.bits = bh->data;
		gp->block_bmp.byte_len = BLOCKS_PER_GROUP / 8;
	}
	if(!(gp->inode_bmp).bits){
		struct buffer_head *bh = read_block(part, gp->inode_bitmap);
		gp->inode_bmp.bits = bh->data;
		gp->inode_bmp.byte_len = INODES_PER_GROUP / 8;
	}
}
