#include "inode.h"
#include "fs.h"
#include "ide.h"
#include "dir.h"

struct dir root_dir;
extern struct partition *cur_par;

void dir_close(struct dir *dir){
	if(dir == root_dir){
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

bool _handle_inode(struct partition *part, struct inode *inode, \
		uint32_t idx, void *buf, uint32_t *block_buf){

	if(idx >= BLOCK_LEVEL_3){
		return false;
	}
	
	uint32_t lba = 0, pre_lba = 0;
	if(idx < BLOCK_LEVEL_0){

		lba = inode->i_sectors[idx];

	}else {
		idx = BLK_IDX(idx);
		uint32_t cnt = BLK_LEVEL(idx);
		if((idx % LBA_PER_BLK) == 0){
			lba = inode->i_sectors[11 + cnt];
			uint32_t i = 1;
			while(i <= cnt){
				if(lba == 0){
					uint32_t new_lba = block_alloc(part);
					if(new_lba == -1){
						return false;
					}
					if(i == 1){

						inode->i_sectors[11 + cnt] = new_lba;
					}else {
						block_buf[BLK_IDX_I(idx, i)] = new_lba;
						ide_write(part, pre_lba, block_buf, SEC_PER_BLK);
					}

					lba = new_lba;
				}
				ide_read(part, lba, block_buf, SEC_PER_BLK);
				pre_lba = lba;
				lba = block_buf[BLK_IDX_I(idx, i)];
				++i;
			}
		}	
	}

	if(!lba)
		return false;

	ide_read(part, lba, buf, SEC_PER_BLK);
	return true;
}

void create_dir_entry(char *filename, uint32_t i_no, enum file_types f_type,\
		struct dir_entry *dir_e){
	memcpy(dir_e->filename, filename, MAX_FILE_NAME_LEN);
	dir_e->i_no = i_no;
	dir_e->f_type = f_type;
}

bool search_dir_entry(struct partition *part, struct dir *dir, \
		const char *name, struct dir_entry *dir_e){

	uint8_t *buf = (uint8_t *)sys_malloc(BLOCK_SIZE);
	uint32_t *block_buf = sys_malloc(BLOCK_SIZE);

	if(!(buf && block_buf)){
		return false;
	}


	uint32_t per_block = BLOCK_SIZE / sizeof(dir_entry);
	uint32_t block_idx = 0;
	uint32_t idx = 0;

	while(_read_inode(part, dir->inode, idx, buf, block_buf)){
		uint32_t dir_entry_idx = 0;
		struct dir_entry *p_de = (struct dir_entry *)buf;
		while(dir_entry_idx < per_block){
			if(!strcmp(name, p_de->filename)){
				memcpy(dir_e, p_de, sizeof(struct dir_entry));
				sys_free(p_de);
				sys_free(block_buf);
				return true;
			}
			++dir_entry_idx;
			++p_de;
		}
		++idx;
		memset(buf, 0, BLOCK_SIZE);
	}

	sys_free(buf);
	sys_free(block_buf);
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
bool add_dir_entry(struct dir *par_dir, struct dir_entry *dir_e, void *buf){

	uint32_t size = sizeof(struct dir_entry);
	struct inode *inode = par_dir->inode;
	uint32_t block_idx = inode->i_size / BLOCK_SIZE;
	uint32_t dir_entry_idx = inode->i_size / size;
	struct dir_entry *buf = (struct dir_entry *)sys_malloc(BLOCK_SIZE);
	uint32_t *block_buf = sys_malloc(BLOCK_SIZE);

	///块内还有剩余，读出块后再写入
	if(inode->i_size % BLOCK_SIZE){
		_read_inode(part, inode, block_idx, buf, block_buf);	
		memcpy(buf + dir_entry_idx, dir_e, siz);
		inode->i_size += size;
		return true;
	}
