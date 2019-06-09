#include "fs.h"
#include "dir.h"
#include "bitmap.h"
#include "string.h"
#include "file.h"

struct dir root_dir;
extern struct partition *cur_par;

void open_root_dir(struct partition *part){
	root_dir.inode = inode_open(part, ROOT_INODE);
	root_dir.dir_pos = 0;
}

void dir_close(struct dir *dir){
	if(dir == &root_dir){
		return;
	}
	inode_close(dir->inode);
	sys_free(dir);
}

struct dir* dir_open(struct partition *part, uint32_t i_no){
	struct dir *dir = (struct dir *)sys_malloc(sizeof(struct dir));
	dir->inode = inode_open(part, i_no);
	dir->dir_pos = 0;
	return dir;
}

/**
 * @brief 读取i节点块，每次读取一个块
 *
 * @param part 分区指针
 * @param inode i节点指针
 * @param idx  该i节点中第idx个块
 * @param buf  输出缓冲
 * @param block_buf  前12个直接块访问不需要使用该变量，间接访问时用于缓存上一级块
 *
 * @return 是否读完
 */

struct buffer_head *_handle_inode(struct partition *part, struct inode_info *inode, uint32_t idx){

	if(idx >= BLOCK_LEVEL_3){
		PANIC("no more space\n");
	}
	
	struct buffer_head *bh = NULL;
	uint32_t blk_nr = 0, pre_blk_nr = 0;

	if(idx < BLOCK_LEVEL_0){
		blk_nr = inode->i_block[idx];

	}else {
		idx = BLK_IDX(idx);
		uint32_t cnt = BLK_LEVEL(idx);

		blk_nr = inode->i_block[MAX_BLOCK_DIR_POS + cnt];

		//TODO 考虑buffer_head 的释放
		uint32_t i = 1;
		while(i <= cnt){
			if(!blk_nr){
				uint32_t new_blk_nr = block_bmp_alloc(part);

				if(i == 1){
					inode->i_block[MAX_BLOCK_DIR_POS + cnt] = new_blk_nr;

				}else {
					bh->data[BLK_IDX_I(idx, i)] = new_blk_nr;
					//写入硬盘
					write_block(part, bh);
					bh = read_block(part, pre_blk_nr);
				}
				blk_nr = new_blk_nr;
			}
			bh = read_block(part, blk_nr);
			pre_blk_nr = blk_nr;
			blk_nr = bh->data[BLK_IDX_I(idx, i)];
			++i;
		}
	}	

	if(!blk_nr)
		return NULL;

	bh = read_block(part, blk_nr);
	return bh;
}

void create_dir_entry(char *filename, uint32_t i_no, enum file_types f_type,\
		struct dir_entry *dir_e){

	memset(dir_entry, 0, sizeof(struct dir_entry));
	memcpy(dir_e->filename, filename, MAX_FILE_NAME_LEN);
	dir_e->i_no = i_no;
	dir_e->f_type = f_type;
}

bool search_dir_entry(struct partition *part, struct dir *dir, \
		const char *name, struct dir_entry *dir_e){

	uint32_t per_block = BLOCK_SIZE / sizeof(struct dir_entry);
	uint32_t idx = 0;
	struct buffer_head *bh = NULL;

	while(bh = _handle_inode(part, dir->inode, idx)){
		uint32_t dir_entry_idx = 0;
		struct dir_entry *p_de = (struct dir_entry *)bh->data;
		while(dir_entry_idx < per_block){
			if(!strcmp(name, p_de->filename)){
				memcpy(dir_e, p_de, sizeof(struct dir_entry));
				return true;
			}
			++dir_entry_idx;
			++p_de;
		}
		++idx;
	}
	return false;
}
			
/**
 * @brief 在目录中添加目录项
 *
 * @param par_dir 指定目录指针
 * @param dir_e 目录项指针
 * @param buf 调用方提供缓冲
 *
 * @reture 是否成功
 */
bool add_dir_entry(struct dir *par_dir, struct dir_entry *dir_e){

	struct inode_info *inode = par_dir->inode;
	struct buffer_head *bh = NULL;
	uint32_t blk_idx = inode->i_blocks;
	uint32_t off_byte = inode->i_size % BLOCK_SIZE;

	//块内没有剩余，这里假定目录项不跨块
	if(!off_byte){
		++blk_idx;
		inode->i_blocks += 1;
	}
	if(blk_idx >= BLOCK_LEVEL_3)
		return false;
	
	bh = _handle_inode(cur_par, inode, blk_idx);
	//返回空指针，说明块不存在
	if(!bh){
		//分配内存
		ALLOC_BH(bh);
		//加入缓冲
		buffer_add_block(&cur_par->io_buffer, bh);
	}

	memcpy((bh->data + off_byte), dir_e, sizeof(struct dir_entry));
	inode->i_size += sizeof(struct dir_entry);
	write_block(cur_par, bh);
	return true;
}


