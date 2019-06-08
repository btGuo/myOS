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

