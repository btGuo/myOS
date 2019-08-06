#include "bitmap.h"
#include "string.h"
#include "inode.h"
#include "ide.h"
#include "dir.h"
#include "block.h"
#include "pathparse.h"
#include "file.h"
#include "memory.h"

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
 * 打开普通目录
 */
struct dir* dir_open(struct partition *part, uint32_t i_no){
	struct dir *dir = (struct dir *)kmalloc(sizeof(struct dir));
	dir->inode = inode_open(part, i_no);
	dir->dir_pos = 0;
	return dir;
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
 * 读目录
 * @param dir 目录指针
 * @param dir_e 读取的目录项
 * @return 是否成功
 */
bool dir_read(struct dir *dir, struct dir_entry *dir_e){

//	printk("dir size %d\n", dir->inode->i_size);
	if(dir->dir_pos == dir->inode->i_size){
		return false;
	}

	uint32_t idx = dir->dir_pos / BLOCK_SIZE;
	uint32_t off_byte = dir->dir_pos % BLOCK_SIZE;
	struct buffer_head *bh = NULL;
	uint32_t blk_nr;

	blk_nr = get_block_num(cur_par, dir->inode, idx, M_SEARCH);
	ASSERT(blk_nr != 0);

	bh = read_block(cur_par, blk_nr);
	memcpy(dir_e, bh->data + off_byte, sizeof(struct dir_entry));
	release_block(bh);
	dir->dir_pos += sizeof(struct dir_entry);
	return true;
}

/**
 * 判断目录是否为空
 */
static bool dir_is_empty(struct dir *dir){
	return (dir->inode->i_size == sizeof(struct dir_entry) * 2);
}

bool delete_dir_entry(struct partition *part, struct dir *par_dir, uint32_t i_no);
/**
 * 删除磁盘上目录
 * @param par_dir 父目录
 */
int32_t dir_remove(struct dir *par_dir, struct dir *dir){
	if(!delete_dir_entry(cur_par, par_dir, dir->inode->i_no))
		return -1;
	inode_delete(cur_par, dir->inode->i_no);
	return 0;
}

/**
 * 初始化目录项
 */
void init_dir_entry(char *filename, uint32_t i_no, enum file_types f_type,\
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
bool _search_dir_entry(struct partition *part, struct dir *dir, \
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
				release_block(bh);
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
 * 提取路径中最后一项目录
 *
 * @param path 目录路径，形式为 /a/b/c/ 最后一个字符是/
 * @note 对于/a/b/c/ 返回c
 */
struct dir *get_last_dir(const char *path){
	
	//根目录直接返回
	if(!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")){
		return &root_dir;
	}

	char name[MAX_FILE_NAME_LEN];
	struct dir *par_dir = &root_dir;
	struct dir_entry dir_e;
	
	while((path = path_parse(path, name))){
		if(_search_dir_entry(cur_par, par_dir, name, &dir_e)){

			dir_close(par_dir);
			if(dir_e.f_type != FT_DIRECTORY){
				printk("%s is not a directory!\n", name);
				return NULL;
			}
			par_dir = dir_open(cur_par, dir_e.i_no);
		}
		else{
			printk("%s is not found!\n", name);
			return NULL;
		}
	}
	return par_dir;
}

/**
 * 根据文件路径搜索文件目录项，如果成功会打开父目录
 * 
 * @param path 路径名
 * @param dir_e 返回的目录项
 *
 * @return 父目录指针
 * 	@retval NULL 查找为空 
 */
struct dir *search_dir_entry(struct partition *part, \
		const char *path, struct dir_entry *dir_e){

	char filename[MAX_FILE_NAME_LEN];
	char dirname[MAX_PATH_LEN];

	split_path(path, filename, dirname);
	struct dir *par_dir = get_last_dir(dirname);

	//父目录不存在
	if(!par_dir){
		printk("search_dir_entry: open directory error\n");
		return NULL;
	}

	//先查找
	bool found = _search_dir_entry(part, par_dir, filename, dir_e);
	if(!found){
		dir_close(par_dir);
		return NULL;
	}
	return par_dir;
}

/**
 * @brief 在目录中添加目录项
 *
 * @param par_dir 指定目录指针
 * @param dir_e 目录项指针
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

/**
 * 删除目录项
 *
 * @param part 分区指针
 * @param par_dir 指定目录指针
 * @param i_no 待删除的目录项的inode号
 *
 * @note 目录项是定长设计，每次删除时会把最后一个补到当前空位上
 * @return 删除是否成功
 */
bool delete_dir_entry(struct partition *part, struct dir *par_dir, uint32_t i_no){

	uint32_t per_block = BLOCK_SIZE / sizeof(struct dir_entry);
	uint32_t idx = 0;
	struct buffer_head *bh = NULL;
	uint32_t blk_nr = 0;;
	uint32_t off_byte = 0;
	bool hit = false;

//先找出要删除的目录项
	while((blk_nr = get_block_num(part, par_dir->inode, idx, M_SEARCH))){
		bh = read_block(part, blk_nr);
		uint32_t dir_entry_idx = 0;
		struct dir_entry *p_de = (struct dir_entry *)bh->data;
		while(dir_entry_idx < per_block){
			if(i_no == p_de->i_no){
				hit = true;
				off_byte = dir_entry_idx * sizeof(struct dir_entry);
				break;
			}
			++dir_entry_idx;
			++p_de;
		}
		if(hit)
			break;
		++idx;
		release_block(bh);
	}
	//找不到
	if(!hit)
		return false;


//找出最后一个目录项，复制到当前位置
	struct inode_info *inode = par_dir->inode;
	int fin_idx = inode->i_blocks - 1;
	//算错了。。。 先减再求模
	int fin_off_byte = (inode->i_size - sizeof(struct dir_entry)) % BLOCK_SIZE;
	struct buffer_head *fin_bh = NULL;

//	printk("fin_off_byte %d\n", fin_off_byte);
//	printk("off_byte %d\n", off_byte);

	//位于同一块内
	if(fin_idx == idx){
	
		fin_bh = bh;	
	}else {
		fin_bh = read_block(part, get_block_num(part, inode, fin_idx, M_SEARCH));
	}

	memcpy((bh->data + off_byte), (fin_bh->data + fin_off_byte), sizeof(struct dir_entry));

	write_block(part, bh);
	release_block(bh);
	if(fin_idx != idx){
		write_block(part, fin_bh);
		release_block(fin_bh);
	}

//检查是否有块已经空，如果有则调用remove_last释放该块
	inode->i_size -= sizeof(struct dir_entry);
	inode->i_blocks = DIV_ROUND_UP(inode->i_size, BLOCK_SIZE);
	if(!(inode->i_size % BLOCK_SIZE))
		remove_last(part, inode);
	return true;
}

//===============================================================
//下面为相关系统调用执行函数
//===============================================================

/**
 * 删除空目录
 */
int32_t sys_rmdir(char *path){

	struct dir_entry dir_e;
	struct dir *par_dir = search_dir_entry(cur_par, path, &dir_e);

	//查找为空
	if(!par_dir){
		printk("%s isn't exist\n", path);
		return -1;
	}
	
	//不是目录
	if(dir_e.f_type == FT_REGULAR){
		printk("%s is not a directory\n", path);
		dir_close(par_dir);
		return -1;
	}

	//目录不是空的
	struct dir *dir = dir_open(cur_par, dir_e.i_no);
	if(!dir_is_empty(dir)){

		printk("dir %s is not empty, it is not allowd to delete\n", path);
		dir_close(par_dir);
		dir_close(dir);
		return -1;
	}

	int32_t retval = dir_remove(par_dir, dir);
	dir_close(par_dir);
	dir_close(dir);
	return retval;
}
	
/**
 * 读取目录
 */
int32_t sys_readdir(struct dir *dir, struct dir_entry *dir_e){
	ASSERT(dir != NULL);
	if(dir_read(dir, dir_e))
		return 0;
	return -1;
}

/**
 * 复位目录偏移
 */
void sys_rewinddir(struct dir *dir){
	dir->dir_pos = 0;
}
			
//这个有点随意
#define FIXUP_PATH(path)\
do{\
	uint32_t len = strlen(path);\
	if(path[len - 1] == '/'){\
		path[len - 1] = 0;\
	}\
}while(0)

/**
 * 创建目录
 * @note 目前只能创建单级目录，注意path可能为/a/b/c/
 */
int32_t sys_mkdir(char *path){

	FIXUP_PATH(path);

	//先查找
	struct dir_entry dir_e;
	char filename[MAX_FILE_NAME_LEN];
	char dirname[MAX_PATH_LEN];

	split_path(path, filename, dirname);
	struct dir *par_dir = get_last_dir(dirname);

	//父目录不存在
	if(!par_dir){
		printk("search_dir_entry: open directory error\n");
		return -1;
	}

	//先查找
	bool found = _search_dir_entry(cur_par, par_dir, filename, &dir_e);

	if(found){
		printk("sys_mkdir: file or directory %s exist!\n", path);
		dir_close(par_dir);
		return -1;
	}

//申请inode
	struct inode_info *m_inode = NULL;
	if(!(m_inode = inode_alloc(cur_par))){

		printk("sys_mkdir: kmalloc for inode failed\n");
		dir_close(par_dir);
		return -1;
	}
	uint32_t i_no = m_inode->i_no;


//在父目录中添加目录项
	init_dir_entry(filename, i_no, FT_DIRECTORY, &dir_e);
	if(!add_dir_entry(par_dir, &dir_e)){
		printk("sys_mkdir: add_dir_entry failed\n");
		dir_close(par_dir);
		inode_release(m_inode);
		return -1;
	}

//先磁盘同步两个inode
	inode_sync(cur_par, par_dir->inode);
	inode_sync(cur_par, m_inode);
	inode_release(m_inode);

	//再打开当前目录
	struct dir *cur_dir = dir_open(cur_par, i_no);

//TODO 考虑磁盘同步问题
	//添加目录项
	init_dir_entry(".",  i_no, FT_DIRECTORY, &dir_e);
	add_dir_entry(cur_dir, &dir_e);
	init_dir_entry("..", par_dir->inode->i_no, FT_DIRECTORY, &dir_e);
	add_dir_entry(cur_dir, &dir_e);

	//同步inode
	inode_sync(cur_par, cur_dir->inode);

	dir_close(par_dir);
	dir_close(cur_dir);

//	printk("sys_mkdir done\n");
	return 0;
}

/**
 * 关闭目录
 */
int32_t sys_closedir(struct dir *dir){

	if(!dir)
		return -1;
	dir_close(dir);
	return 0;
}

/**
 * 打开目录
 */
struct dir *sys_opendir(char *path){

	if(path[0] == '/' && path[1] == 0){
		return &root_dir;
	}

	FIXUP_PATH(path);

	//先查找
	struct dir_entry dir_e;
	struct dir *par_dir = search_dir_entry(cur_par, path, &dir_e);

	if(par_dir){
		dir_close(par_dir);
		if(dir_e.f_type == FT_REGULAR){
			printk("%s is a regular file\n", path);
			return NULL;
		}
		else{
			return dir_open(cur_par, dir_e.i_no);
		}
	}

	printk("%s isn't exist\n", path);
	return NULL;
}	
