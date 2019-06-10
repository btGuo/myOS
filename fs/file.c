#include "file.h"
#include "ide.h"
#include "bitmap.h"
#include "group.h"
#include "super_block.h"
#include "thread.h"
#include "fs.h"


struct file file_table[MAX_FILE_OPEN];   ///< 文件表
extern struct task_struct *curr;

/**
 * @brief 在文件表中找到空位
 */
int32_t get_fd(){
	uint32_t idx = 3;
	while(idx < MAX_FILE_OPEN){
		if(file_table[idx].fd_inode == NULL){
			return idx;
		}
		++idx;
	}
	return -1;
}

/**
 * @brief 在当前进程文件表中安装文件
 */
int32_t set_fd(int32_t fd){
	int idx = 3;
	while(idx < MAX_FILES_OPEN_PER_PROC){
		if(curr->fd_table[idx] == -1){
			curr->fd_table[idx] = fd;
			return idx;
		}
		++idx;
	}
	return -1;
}

/**
 * 分配inode块
 *
 * @return 新分配inode号
 * 	@retval -1 分配失败
 */
int32_t inode_bmp_alloc(struct partition *part){
	struct group_info *cur_gp = part->cur_gp;
	struct super_block *sb = part->sb;

	ASSERT(sb->free_inodes_count > 0);
	//当前组中已经没有空闲位置，切换到下一个组
	if(cur_gp->free_inodes_count == 0){
		cur_gp = cur_gp->next;
		group_info_init(part, cur_gp);
		part->cur_gp = cur_gp;
	}
	//注意减1
	--cur_gp->free_inodes_count;
	--sb->free_inodes_count;

	uint32_t idx = bitmap_scan(&cur_gp->inode_bmp, 1);
	bitmap_set(&cur_gp->inode_bmp, idx, 1);
	return idx + cur_gp->group_nr * INODES_PER_GROUP;
}

/**
 * 复位inode节点
 */
void inode_bmp_clear(struct partition *part, uint32_t i_no){
	struct group_info *gp = part->groups + i_no / INODES_PER_GROUP;
	i_no %= INODES_PER_GROUP;
	bitmap_set(&gp->inode_bmp, i_no, 0);
}

/**
 * 分配block块
 *
 * @return 新分配块的块号
 * 	@retval -1 分配失败
 */
int32_t block_bmp_alloc(struct partition *part){
	struct group_info *cur_gp = part->cur_gp;
	struct super_block *sb = part->sb;

	ASSERT(sb->free_blocks_count > 0);
	//当前组中已经没有空闲位置，切换到下一个组
	if(cur_gp->free_blocks_count == 0){
		cur_gp = cur_gp->next;
		group_info_init(part, cur_gp);
		part->cur_gp = cur_gp;
	}
	//注意减1
	--cur_gp->free_blocks_count;
	--sb->free_blocks_count;

	uint32_t idx = bitmap_scan(&cur_gp->block_bmp, 1);
	bitmap_set(&cur_gp->block_bmp, idx, 1);
	return idx + cur_gp->group_nr * BLOCKS_PER_GROUP;
}

/**
 * 在块位图中复位
 */

void block_bmp_clear(struct partition *part, uint32_t blk_nr){
	struct group_info *gp = part->groups + blk_nr / BLOCKS_PER_GROUP;
	blk_nr %= BLOCKS_PER_GROUP;
	bitmap_set(&gp->block_bmp, blk_nr, 0);
}

/**
 * 创建文件
 *
 * @param par_dir 新建文件的父目录
 * @param filename 文件名
 * @param flag 文件标志位
 *
 * @return 文件的文件号
 */
int32_t file_create(struct dir *par_dir, char *filename, uint8_t flag){
	
	//分配inode号
	int32_t i_no = inode_bmp_alloc(cur_par);
	uint32_t roll_no = 0;
	struct inode_info *m_inode = (struct inode_info *)sys_malloc(sizeof(struct inode_info));
	if(!m_inode){
		//内存分配失败，回滚
		roll_no = 1;
		printk("file_create: sys_malloc for inode failed\n");
		goto rollback;
	}

	//初始化inode
	inode_init(cur_par, m_inode, i_no);

	//获取文件号
	int32_t fd_idx = get_fd();
	if(fd_idx == -1){
		//达到最大文件打开数，回滚
		printk("exceed max open files\n");
		roll_no = 2;
		goto rollback;
	}

	//在文件表中安装
	file_table[fd_idx].fd_pos = 0;
	file_table[fd_idx].fd_flag = flag;
	file_table[fd_idx].fd_inode = m_inode;
	
	//申请并安装目录项
	struct dir_entry dir_entry;
	create_dir_entry(filename, i_no, FT_REGULAR, &dir_entry);
	if(!add_dir_entry(par_dir, &dir_entry)){
		printk("sync dir_entry to disk failed\n");
		roll_no = 3;
		goto rollback;
	}
	//磁盘同步两个inode
	inode_sync(cur_par, par_dir->inode);
	inode_sync(cur_par, m_inode);
	return set_fd(fd_idx);

rollback:
	switch(roll_no){
		case 3:
			memset(&file_table[fd_idx], 0 sizeof(struct file));
		case 2:
			sys_free(m_inode);
		case 1:
			inode_bmp_clear(cur_par, i_no);
			break;
	}
	return -1;
}

int32_t file_open(uint32_t i_no, uint8_t flag){
	int fd_idx = get_fd();
	if(fd_idx == -1){
		printk("exceed max open files\n");
		return -1;
	}

	file_table[fd_idx].fd_inode = inode_open(cur_par, i_no);
	file_table[fd_idx].fd_pos = 0;
	file_table[fd_idx].fd_flag = flag;
	bool *write_deny = &file_table[fd_idx].fd_inode->i_write_deny;

	if(flag & O_WRONLY || flag & O_RDWR){
		enum intr_status old_stat = intr_disable();
		if(!(*write_deny)){
			*write_deny = true;
			intr_set_status(old_stat);
		}else {
			intr_set_status(old_stat);
			printk("file can't be write now, try again later\n");
			return -1;
		}
	}
	return set_fd(fd_idx);
}


		
		

