#include "inode.h"
#include "global.h"
#include "ide.h"
#include "string.h"
#include "fs.h"
#include "buffer.h"
#include "group.h"
#include "interrupt.h"
#include "super_block.h"

extern struct task_struct *curr;
extern struct partition *cur_par;
/**
 * @brief 定位inode
 *
 * @param i_no inode号
 * @param pos  输出结果
 */
void inode_locate(struct partition *part, uint32_t i_no, struct inode_pos *pos){
	ASSERT(i_no < part->sb->inodes_count);
	struct group *gp = GROUP_PTR(&part->groups, i_no);
	uint32_t offset = (i_no % INODES_PER_GROUP) * sizeof(struct inode);
	
	pos->blk_nr = gp->inode_bitmap;
	pos->off_size = offset % SECTOR_SIZE;
}

/**
 * @brief 释放inode
 * @detail 如果位于缓冲中，则交给缓冲区管理，如果不是
 * 则写入磁盘后，释放内存
 */
void release_inode(struct inode_info *m_inode){
	if(m_inode->i_buffered){
		BUFW_INODE(m_inode);
		return;
	}
	write_inode(m_inode);
	sys_free(m_inode);
}

/**
 * @brief 将inode写入磁盘
 */
void write_inode(struct inode_info *m_inode){
	//定位inode
	inode_locate(cur_par, m_inode->i_no, &pos);
	bh = read_block(cur_par, pos.blk_nr);
	memcpy((bh->data + pos.off_size), &m_inode, sizeof(struct inode));
	write_block(cur_par, bh);
	//复位脏
	m_inode->i_dirty = false;
}

/** not used */
void inode_info_init(struct inode_info *m_inode, struct inode *d_inode, uint32_t i_no){
	memcpy(m_inode, d_inode, sizeof(struct inode));
	m_inode->i_no = i_no;
	m_inode->i_blocks = m_inode->i_size / BLOCK_SIZE;
}

/**
 * @brief 根据索引节点号，打开索引节点
 */
struct inode_info *inode_open(struct partition *part, uint32_t i_no){
	struct inode_info *m_inode = buffer_read_inode(&part->io_buffer, i_no);
	//缓冲区命中，增加引用计数后返回
	if(m_inode){
		++m_inode->i_open_cnts;
		return m_inode;
	}
	
	//缓冲区不命中
	struct inode_pos pos;
	struct buffer_head *bh;

	m_inode = (struct inode_info *)sys_malloc(sizeof(struct inode_info));
	MEMORY_OK(m_inode);

	inode_locate(part, i_no, &pos);
	bh = read_block(&part->io_buffer, pos.blk_nr);
	memcpy((bh->data + pos.off_size), &m_inode, sizeof(struct inode));
	//初始化
	m_inode->i_no = i_no;
	m_inode->i_open_cnts = 0;
	m_inode->i_lock = true;
	m_inode->i_buffered = true;
	//加入缓冲区
	if(!buffer_add_inode(&part->io_buffer, m_inode)){
		//缓冲区已满
		m_inode->i_buffered = false;
	}

	return m_inode;
}

void inode_close(struct inode_info *m_inode){
	ASSERT(m_inode->i_open_cnts > 0);

	//关中断，防止重复释放
	enum intr_status old_stat = intr_disable();
	//引用计数为0则释放inode
	if(--m_inode->i_open_cnts == 0){
		m_inode->i_lock = false;
		release_inode(m_inode);
	}
	intr_set_status(old_stat);
}

void inode_init(struct inode_info *m_inode, uint32_t i_no){
	memset((void *)m_inode, 0, sizeof(struct inode_info));
	m_inode->i_no = i_no;
}

