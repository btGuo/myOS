#include "file.h"
#include "ide.h"

struct file file_table[MAX_FILE_OPEN];   ///< 文件表

/**
 * 在文件表中找到空位
 *
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

int32_t set_fd(int32_t fd){
	int idx = 3;
	while(idx < MAX_FILES_OPEN_PER_PROC){
		if(curr->fd_table[idx] == -1){
			return idx;
		}
		++idx;
	}
	return -1;
}

int32_t inode_bmp_alloc(struct partition *part){
	struct group_info *cur_gp = part->cur_gp;
	struct super_block *sb = part->sb;

	ASSERT(sb->free_inodes_count > 0);
	if(cur_gp->free_inodes_count == 0){
		cur_gp = cur_gp->next;
		group_info_init(cur_gp);
		part->cur_gp = cur_gp;
	}
	--cur_gp->free_inodes_count;
	--sb->free_inodes_count;

	uint32_t idx = bitmap_scan(&cur_gp->inode_bmp, 1);
	if(idx != -1){
		bitmap_set(&cur_gp->inode_bmp, idx, 1);
	}
	return idx + cur_gp->group_nr * INODES_PER_GROUP;
}

/**
 * 分配block块
 *
 * @return 新分配块的lba地址
 * 	@retval -1 分配失败
 */
int32_t block_alloc(struct partition *part){
	int32_t idx = bitmap_scan(&part->block_bitmap, 1);
	if(idx != -1){
		bitmap_set(&part->block_bitmap, idx, 1);
		return part->sb->data_start_lba + idx * BLOCK_SIZE;
	}
	return idx;
}


