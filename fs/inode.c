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
#include "block.h"
#include "memory.h"
#include "group.h"
#include "bitmap.h"

extern struct task_struct *curr;
extern struct partition *cur_par;


/**
 * 分配inode块
 *
 * @return 新分配inode号
 * 	@retval -1 分配失败
 */
int32_t inode_bmp_alloc(struct partition *part){
	struct group_info *cur_gp = part->cur_gp;
	struct super_block *sb = part->sb;

	if(sb->free_inodes_count <= 0)
		return -1;
	//当前组中已经没有空闲位置，切换到下一个组
	if(cur_gp->free_inodes_count == 0){

		cur_gp = group_switch(part);
	}
	//注意减1
	--cur_gp->free_inodes_count;
	--sb->free_inodes_count;

	uint32_t idx = bitmap_scan(&cur_gp->inode_bmp, 1);
	bitmap_set(&cur_gp->inode_bmp, idx, 1);
	return idx + cur_gp->group_nr * INODES_PER_GROUP;
}

/**
 * 复位inode位图
 */
void inode_bmp_clear(struct partition *part, uint32_t i_no){
	struct group_info *gp = part->groups + i_no / INODES_PER_GROUP;
	i_no %= INODES_PER_GROUP;
	bitmap_set(&gp->inode_bmp, i_no, 0);
}

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

/**
 * 根据索引节点号，打开索引节点，如果不在内存则将磁盘上的inode加载到内存
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
 * 关闭inode，与inode_open对应
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
 * 申请新的inode，并分配内存，初始化
 */
bool inode_alloc(struct partition *part, struct inode_info *m_inode){
	int32_t i_no = inode_bmp_alloc(part);
	if(i_no < 0){
		return false;
	}
	m_inode = (struct inode_info *)sys_malloc(sizeof(struct inode_info));
	if(!m_inode){
		inode_bmp_clear(part, i_no);
		return false;
	}
	//这里应该清零，内存可能不干净
	memset(m_inode, 0, sizeof(struct inode_info));
	//初始化
	inode_init(part, m_inode, i_no);
	return true;
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

	ASSERT(m_inode->i_open_cnts == 1);
	inode_bmp_clear(part, i_no);

	//清空对应的所有块内容
	clear_blocks(part, m_inode);
	//脏位设置为假，不用写入了
	m_inode->i_dirty = false;
	inode_close(m_inode);
}
		
