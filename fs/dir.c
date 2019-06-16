#include "fs.h"
#include "dir.h"
#include "bitmap.h"
#include "string.h"
#include "file.h"

struct dir root_dir;
extern struct partition *cur_par;

void print_root(struct inode_info *m_inode){
	printk("root inode info\n");
	printk("%d  %d  %d\n", m_inode->i_block[0], m_inode->i_size, m_inode->i_blocks);
}

/**
 * 打开根目录
 */
void open_root_dir(struct partition *part){
	root_dir.inode = inode_open(part, ROOT_INODE);
	root_dir.dir_pos = 0;
	print_root(root_dir.inode);
}

/**
 * 关闭目录
 */
void dir_close(struct dir *dir){
	if(dir == &root_dir){
		return;
	}
	inode_close(dir->inode);
	sys_free(dir);
}

/**
 * 打开普通目录
 */
struct dir* dir_open(struct partition *part, uint32_t i_no){
	struct dir *dir = (struct dir *)sys_malloc(sizeof(struct dir));
	dir->inode = inode_open(part, i_no);
	dir->dir_pos = 0;
	return dir;
}

//有待debug
/**
 * 将文件内的块号转换为实际块号
 *
 * @param part 分区指针
 * @param inode i节点指针
 * @param idx  该i节点中第idx个块
 * @param mode M_SEARCH 或者 M_CREATE 搜索或者创建，
 * 如果为创建数据块不在时则会新分配块号
 *
 * @return 物理块号
 * 	@retval 0 查找失败
 */

uint32_t get_block_num(struct partition *part, struct inode_info *inode,\
	       	uint32_t idx, uint8_t mode){

	if(idx >= BLOCK_LEVEL_3){
		PANIC("no more space\n");
	}
	
	struct buffer_head *bh = NULL;
	uint32_t blk_nr = 0;

	if(idx < BLOCK_LEVEL_0){
		blk_nr = inode->i_block[idx];
		//TODO 磁盘不干净怎么办
		if(blk_nr == 0 || blk_nr < 0 || blk_nr > BLOCK_LEVEL_3){

			//搜索为空，直接返回
			if(mode == M_SEARCH)
				return 0;

			uint32_t new_blk_nr = block_bmp_alloc(part);
			printk("new blk_nr %d\n", new_blk_nr);
			inode->i_block[idx] = new_blk_nr;
			blk_nr = new_blk_nr;
		}
	}else {
		idx = BLK_IDX(idx);
		//间接次数
		uint32_t cnt = BLK_LEVEL(idx);

		blk_nr = inode->i_block[MAX_BLOCK_DIR_POS + cnt];
		//基地址不存在，单独处理
		if(blk_nr == 0 || blk_nr < 0 || blk_nr > BLOCK_LEVEL_3){
			uint32_t new_blk_nr = block_bmp_alloc(part);
			inode->i_block[MAX_BLOCK_DIR_POS + cnt] = new_blk_nr;
			blk_nr = new_blk_nr;
		}

		uint32_t i = 1;
		uint32_t *pos = NULL;
		while(i <= cnt){

			bh = read_block(part, blk_nr);
			pos = (uint8_t *)&bh->data[BLK_IDX_I(idx, i)];
			blk_nr = *pos;

			//块不存在
			if(!blk_nr){
				//最后一块为数据块
				if(i == cnt && mode == M_SEARCH)
					return 0;
				uint32_t new_blk_nr = block_bmp_alloc(part);

				*pos = new_blk_nr;
				//写入硬盘
				write_block(part, bh);
				blk_nr = new_blk_nr;
			}

			//释放
			release_block(bh);
			++i;
		}
	}	
	return blk_nr;
}

/**
 * 初始化目录项
 */
void create_dir_entry(char *filename, uint32_t i_no, enum file_types f_type,\
		struct dir_entry *dir_e){

	memset(dir_e, 0, sizeof(struct dir_entry));
	memcpy(dir_e->filename, filename, MAX_FILE_NAME_LEN);
	dir_e->i_no = i_no;
	dir_e->f_type = f_type;
}

/**
 * 搜索目录
 * @param dir 待搜索的目录
 * @param name 文件或目录名
 * @param dir_e 返回的目录项
 *
 * @return 搜索结果
 * 	@retval false 失败
 */
bool search_dir_entry(struct partition *part, struct dir *dir, \
		const char *name, struct dir_entry *dir_e){

	uint32_t per_block = BLOCK_SIZE / sizeof(struct dir_entry);
	uint32_t idx = 0;
	struct buffer_head *bh = NULL;
	uint32_t blk_nr;

	while((blk_nr = get_block_num(part, dir->inode, idx, M_SEARCH))){
		bh = read_block(part, blk_nr);
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
		release_block(bh);
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
	//最后一块可用块，可能已经满了
	//这里用有符号的,第一块时是-1
	int32_t blk_idx = inode->i_blocks - 1;    
	uint32_t off_byte = inode->i_size % BLOCK_SIZE;

	if(blk_idx >= BLOCK_LEVEL_3)
		return false;
	//块内没有剩余，这里假定目录项不跨块
	if(!off_byte){
		++blk_idx;
		inode->i_blocks += 1;
		if(blk_idx >= BLOCK_LEVEL_3)
			return false;
	}
	
	//printk("blk_idx %d\n", blk_idx);
	uint32_t blk_nr = get_block_num(cur_par, inode, blk_idx, M_CREATE);
	bh = read_block(cur_par, blk_nr);

	memcpy((bh->data + off_byte), dir_e, sizeof(struct dir_entry));
	inode->i_size += sizeof(struct dir_entry);
//	printk("%d\n", inode->i_size);
	write_block(cur_par, bh);
	release_block(bh);
	return true;
}
