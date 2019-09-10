#include "group.h"
#include "buffer.h"
#include "fs.h"
#include "block.h"
#include "local.h"

/**
 * 初始化组描述符，如果位图不在内存中则加载到内存并初始化
 */
void group_info_init(struct fext_fs *fs, struct fext_group_m *gp){
	if(!gp->block_bmp_bh){
		gp->block_bmp_bh = read_block(fs, gp->block_bitmap);
		gp->block_bmp.bits = gp->block_bmp_bh->data;
		gp->block_bmp.byte_len = BLOCKS_PER_GROUP / 8;
	}
	if(!gp->inode_bmp_bh){
		gp->inode_bmp_bh = read_block(fs, gp->inode_bitmap);
		gp->inode_bmp.bits = gp->inode_bmp_bh->data;
		gp->inode_bmp.byte_len = INODES_PER_GROUP / 8;
	}
}

/**
 * 切换组
 */
struct fext_group_m *group_switch(struct fext_fs *fs){
	
	struct fext_group_m *cur_gp = fs->cur_gp;

	//写入前一组位图
	write_block(fs, cur_gp->block_bmp_bh);
	write_block(fs, cur_gp->inode_bmp_bh);
	//释放
	release_block(cur_gp->block_bmp_bh);
	release_block(cur_gp->inode_bmp_bh);

	//这里直接加一，因为在内存上是连续的，要特别注意小心越界
	++fs->cur_gp;
	//防止溢出
	ASSERT(fs->cur_gp != fs->groups + fs->groups_cnt);
	group_info_init(fs, fs->cur_gp);
	return fs->cur_gp;
}

/**
 * 同步当前分区组描述符位图
 */
void group_bmp_sync(struct fext_fs *fs){

	struct fext_group_m *cur_gp = fs->cur_gp;

	//写入当前组
	write_block(fs, cur_gp->block_bmp_bh);
	write_block(fs, cur_gp->inode_bmp_bh);
	//释放
	release_block(cur_gp->block_bmp_bh);
	release_block(cur_gp->inode_bmp_bh);
}
