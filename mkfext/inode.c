#include "inode.h"
#include "fs.h"
#include "buffer.h"
#include "group.h"
#include "super_block.h"
#include "dir.h"
#include "block.h"
#include "group.h"
#include "bitmap.h"

#include "local.h"

/**
 * 分配inode块
 *
 * @return 新分配inode号
 * 	@retval -1 分配失败
 */
int32_t inode_bmp_alloc(struct fext_fs *fs){
	struct fext_group_m *cur_gp = fs->cur_gp;
	struct fext_super_block *sb = fs->sb;

	if(sb->free_inodes_count <= 0)
		return -1;
	//当前组中已经没有空闲位置，切换到下一个组
	if(cur_gp->free_inodes_count == 0){

		cur_gp = group_switch(fs);
	}
	//注意减1
	--cur_gp->free_inodes_count;
	--sb->free_inodes_count;

	uint32_t idx = bitmap_scan(&cur_gp->inode_bmp, 1);
	bitmap_set(&cur_gp->inode_bmp, idx, 1);
	return idx + cur_gp->group_nr * fs->sb->inodes_per_group;
}

/**
 * 复位inode位图
 */
void inode_bmp_clear(struct fext_fs *fs, uint32_t i_no){
	struct fext_group_m *gp = fs->groups + i_no / fs->sb->inodes_per_group;
	i_no %= fs->sb->inodes_per_group;
	bitmap_set(&gp->inode_bmp, i_no, 0);
}
/**
 * @brief 定位inode
 *
 * @param i_no inode号
 * @param pos  输出结果
 */
void inode_locate(struct fext_fs *fs, uint32_t i_no, struct fext_inode_pos *pos){
	ASSERT(i_no < fs->sb->inodes_count);
	struct fext_group_m *gp = fext_group_ptr(fs, i_no);
	uint32_t offset = (i_no % fs->sb->inodes_per_group) * sizeof(struct fext_inode);
	uint32_t block_size = fs->sb->block_size;
	
	pos->blk_nr = gp->inode_table + offset / block_size ;
	pos->off_size = offset % block_size;

}

/**
 * @brief 释放inode内存
 */
void inode_release(struct fext_inode_m *m_inode){

	if(m_inode->i_buffered){
		BUFR_INODE(m_inode);
		return;
	}
	free(m_inode);
}

/**
 * @brief 磁盘同步inode
 * @detail 如果位于缓冲中，则交给缓冲区管理，如果不是
 * 则写入磁盘
 */
void inode_sync(struct fext_inode_m *m_inode){
	if(m_inode->i_buffered){
		BUFW_INODE(m_inode);
		return;
	}

	struct fext_fs *fs = m_inode->fs;
	struct fext_inode_pos pos;
	struct buffer_head *bh;
	inode_locate(fs, m_inode->i_no, &pos);
	bh = read_block(fs, pos.blk_nr);
	memcpy((bh->data + pos.off_size), m_inode, sizeof(struct fext_inode));
	write_block(fs, bh);
	release_block(bh);
}

/**
 * 根据索引节点号，打开索引节点，如果不在内存则将磁盘上的inode加载到内存
 */
struct fext_inode_m *inode_open(struct fext_fs *fs, uint32_t i_no){
	struct fext_inode_m *m_inode = buffer_read_inode(&fs->io_buffer, i_no);
	//缓冲区命中，增加引用计数后返回
	if(m_inode){
		++m_inode->i_open_cnts;
		return m_inode;
	}
	
	//缓冲区不命中
	struct fext_inode_pos pos;
	struct buffer_head *bh;

	m_inode = (struct fext_inode_m *)malloc(sizeof(struct fext_inode_m));
	MEMORY_OK(m_inode);

	//定位后读出块
	inode_locate(fs, i_no, &pos);
	bh = read_block(fs, pos.blk_nr);
	memcpy(m_inode, (bh->data + pos.off_size), sizeof(struct fext_inode));
	release_block(bh);
	//初始化
	inode_init(fs, m_inode, i_no);
	++m_inode->i_open_cnts;


	return m_inode;
}

/**
 * 关闭inode，与inode_open对应
 */
void inode_close(struct fext_inode_m *m_inode){
	
	if(m_inode->i_no == 0 || m_inode->i_mounted){
		return;
	}

	ASSERT(m_inode->i_open_cnts > 0);
	m_inode->i_write_deny = false;

	if(--m_inode->i_open_cnts == 0){
		inode_release(m_inode);
	}
}

/**
 * 申请新的inode，并分配内存，初始化
 */
struct fext_inode_m *inode_alloc(struct fext_fs *fs){
	int32_t i_no = inode_bmp_alloc(fs);
	if(i_no < 0){
		return NULL;
	}
	struct fext_inode_m *m_inode = 
		(struct fext_inode_m *)malloc(sizeof(struct fext_inode_m));
	if(!m_inode){
		inode_bmp_clear(fs, i_no);
		return NULL;
	}
	//这里应该清零，内存可能不干净
	memset(m_inode, 0, sizeof(struct fext_inode_m));
	//初始化
	inode_init(fs, m_inode, i_no);
	return m_inode;
}

/**
 * 初始化inode，并尝试加入缓冲区，这里只初始化了内存中的部分
 */
void inode_init(struct fext_fs *fs, struct fext_inode_m *m_inode, uint32_t i_no){

	//对块大小上取整
	m_inode->i_blocks = DIV_ROUND_UP(m_inode->i_size, fs->sb->block_size);
	m_inode->i_no = i_no;
	m_inode->i_open_cnts = 0;
	m_inode->i_dirty = true;
	m_inode->i_lock = true;  //这里上锁
	m_inode->i_buffered = true;
	m_inode->i_write_deny = false;
	m_inode->fs = fs;
	m_inode->i_mounted = false;
	//LIST_HEAD_INIT(m_inode->hash_tag);
	//LIST_HEAD_INIT(m_inode->queue_tag);

	//根节点不需要加入缓冲
	if(!i_no){
		m_inode->i_buffered = false;
		return;
	}
	m_inode->i_buffered = buffer_add_inode(&fs->io_buffer, m_inode);
}
