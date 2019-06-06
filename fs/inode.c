#include "inode.h"
#include "global.h"
#include "ide.h"
#include "string.h"
#include "fs.h"
#include "interrupt.h"

extern struct task_struct *curr;

struct inode_pos{
	uint32_t blk_nr;
	uint32_t off_size;
};

static void inode_locate(struct partition *part, uint32_t i_no, struct inode_pos *pos){
	ASSERT(i_no < MAX_FILES_PER_PART);
	struct group *gp = GROUP_PTR(&part->groups, i_no);
	uint32_t offset = (i_no % INODES_PER_GROUP) * sizeof(struct inode);
	
	pos->blk_nr = gp->inode_bitmap;
	pos->off_size = offset % SECTOR_SIZE;
}

void inode_info_init(struct inode_info *m_inode, struct inode *d_inode, uint32_t i_no){
	memcpy(m_inode, d_inode, sizeof(d_inode));
	inode->i_no = i_no;
	inode->i_blocks = i_size / BLOCK_SIZE;
}

void inode_sync(struct partition *part, struct inode *inode, void *io_buf){
	struct inode_pos pos;
	inode_locate(part, inode->i_no, &pos);
	struct inode d_inode;
	memcpy(&d_inode, inode, sizeof(struct inode));
	d_inode.i_open_cnts = 0;
	d_inode.write_deny = false;
	d_inode.inode_tag.prev = d_inode.inode_tag.next = NULL;

	char *buf = (char *)io_buf;

	ide_read(part->disk, pos->sec_lba, buf, 1);
	memcpy((buf + pos->off_size), &d_inode, sizeof(struct inode));
	ide_write(part->disk, pos->sec_lba, buf, 1);
}

/**
 * @brief 根据索引节点号，打开索引节点
 */
struct inode_info *inode_open(struct partition *part, uint32_t i_no){
	return buffer_get_inode(&part->d_buf, i_no);
}

void inode_close(struct inode_info *m_inode){
	ASSERT(m_inode->i_open_cnts > 0);

	enum intr_status old_stat = intr_disable();
	if(--m_inode->i_open_cnts == 0){

	}
	intr_set_status(old_stat);
}

void inode_init(uint32_t i_no, struct inode *inode){
	memset((void *)inode, 0, sizeof(struct inode));
	inode->i_no = i_no;
}


