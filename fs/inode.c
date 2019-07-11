#include "inode.h"
#include "global.h"
#include "ide.h"
#include "string.h"
#include "fs.h"
#include "buffer.h"
#include "group.h"
#include "interrupt.h"
#include "super_block.h"
#include "dir.h"

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
	struct group_info *gp = GROUP_PTR(part->groups, i_no);
	uint32_t offset = (i_no % INODES_PER_GROUP) * sizeof(struct inode);
	
	pos->blk_nr = gp->inode_table + offset / BLOCK_SIZE ;
	pos->off_size = offset % BLOCK_SIZE;
//	printk("pos->blo_nr %d pos->off_size %d\n", pos->blk_nr, pos->off_size);
}

/**
 * @brief 释放inode内存
 */
void inode_release(struct inode_info *m_inode){

	if(m_inode->i_buffered){
		BUFR_INODE(m_inode);
		return;
	}
	sys_free(m_inode);
}

/**
 * @brief 磁盘同步inode
 * @detail 如果位于缓冲中，则交给缓冲区管理，如果不是
 * 则写入磁盘
 */
void inode_sync(struct partition *part, struct inode_info *m_inode){
	if(m_inode->i_buffered){
		BUFW_INODE(m_inode);
		return;
	}
	struct inode_pos pos;
	struct buffer_head *bh;
	inode_locate(part, m_inode->i_no, &pos);
	bh = read_block(part, pos.blk_nr);
	memcpy((bh->data + pos.off_size), m_inode, sizeof(struct inode));
	write_block(part, bh);
	release_block(bh);
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

	//定位后读出块
	inode_locate(part, i_no, &pos);
	bh = read_block(part, pos.blk_nr);
	memcpy(m_inode, (bh->data + pos.off_size), sizeof(struct inode));
	release_block(bh);
	//初始化
	inode_init(part, m_inode, i_no);
	++m_inode->i_open_cnts;


	return m_inode;
}

/**
 * 关闭inode
 */
void inode_close(struct inode_info *m_inode){
	ASSERT(m_inode->i_open_cnts > 0);
	m_inode->i_write_deny = false;

	//关中断，防止重复释放
	enum intr_status old_stat = intr_disable();
	//引用计数为0则释放inode
	if(--m_inode->i_open_cnts == 0){
		inode_release(m_inode);
	}
	intr_set_status(old_stat);
}

/**
 * 初始化inode，并尝试加入缓冲区，这里只初始化了内存中的部分
 */
void inode_init(struct partition *part, struct inode_info *m_inode, uint32_t i_no){

	//对块大小上取整
	m_inode->i_blocks = DIV_ROUND_UP(m_inode->i_size, BLOCK_SIZE);
	m_inode->i_no = i_no;
	m_inode->i_open_cnts = 0;
	m_inode->i_dirty = true;
	m_inode->i_lock = true;  //这里上锁
	m_inode->i_buffered = true;
	m_inode->i_write_deny = false;
	LIST_HEAD_INIT(m_inode->hash_tag);
	LIST_HEAD_INIT(m_inode->queue_tag);

	//根节点不需要加入缓冲
	if(!i_no){
		m_inode->i_buffered = false;
		return;
	}
	m_inode->i_buffered = buffer_add_inode(&part->io_buffer, m_inode);
}

/**
 * 删除磁盘上inode
 */
void inode_delete(struct partition *part, uint32_t i_no){

	struct inode_info *m_inode = inode_open(part, i_no);
	inode_bmp_clear(part, i_no);

	//清空对应的所有块内容
	clear_blocks(part, m_inode);
	//脏位设置为假，不用写入了
	m_inode->i_dirty = false;
	m_inode->i_lock = false;
	inode_close(m_inode);
}

		
