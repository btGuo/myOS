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
	int32_t idx = bitmap_scan(&part->inode_bitmap, 1);
	if(idx != -1){
		bitmap_set(&part->inode_bitmap, idx, 1);
	}
	return idx;
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


