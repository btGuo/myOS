#include "inode.h"
#include "global.h"
#include "ide.h"
#include "string.h"
#include "fs.h"

extern struct task_struct *curr;

struct inode_pos{
	uint32_t sec_lba;
	uint32_t off_size;
};

static void inode_locate(struct partition *part, uint32_t i_no, struct inode_pos *pos){
	ASSERT(i_no < MAX_FILES_PER_PART);
	uint32_t offset = i_no * sizeof(struct inode);
	pos->sec_lba = part->sb->inode_table_lba + offset / SECTOR_SIZE;
	pos->off_size = offset % SECTOR_SIZE;
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

struct inode * inode_open(struct partition *part, uint32_t i_no){

	struct inode *inode = NULL;
	uint32_t len = list_len(&part->open_inodes);
	struct list_head *elem = &part->open_inodes;
	while(len--){
		elem = elem->next;
		inode = list_entry(struct inode, inode_tag, elem);
		if(inode->i_no == i_no){
			inode->i_open_cnts++;
			return inode;
		}
	}

	struct inode_pos pos;
	inode_locate(part, i_no, &pos);
	uint32_t *pgdir_bak = curr->pg_dir;
	curr->pg_dir = NULL;

	inode = (struct inode *)sys_malloc(sizeof(struct inode));
	curr->pg_dir = pgdir_bak;

	char *buf = sys_malloc(SECTOR_SIZE);
	ide_read(part->disk, pos->sec_lba, buf, 1);
	memcpy(inode, (buf + pos->off_size), sizeof(struct inode));

	list_add(&inode->inode_tag, &part->open_inodes);
	sys_free(buf);
	return inode;

}

void inode_close(struct inode *inode){



	
