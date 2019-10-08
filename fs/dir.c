#include "bitmap.h"
#include "string.h"
#include "inode.h"
#include "ide.h"
#include "dir.h"
#include "block.h"
#include "pathparse.h"
#include "file.h"
#include "memory.h"
#include "super_block.h"

#ifdef DEBUG
void print_root(struct fext_inode_m *m_inode){
	printk("root inode info\n");
	printk("%d\t%d\t%d\n", 
		m_inode->i_block[0], 
		m_inode->i_size, 
		m_inode->i_blocks
	);
}
#endif

void dirent_info_init(struct dirent_info *dire_i){
	LIST_HEAD_INIT(dire_i->hash_tag);
	LIST_HEAD_INIT(dire_i->parent_tag);
	LIST_HEAD_INIT(dire_i->child_list);
}

/**
 * 打开普通目录
 */
DIR* dir_open(struct fext_fs *fs, uint32_t i_no){
	DIR *dir = (DIR *)kmalloc(sizeof(DIR));
	dir->inode = inode_open(fs, i_no);
	dir->buffer = NULL;
	dir->count = 0;
	dir->current = 0;
	dir->eof = false;
	return dir;
}
/**
 * 新建目录
 */
DIR* dir_new(struct fext_inode_m *inode){
	DIR *dir = (DIR *)kmalloc(sizeof(DIR));
	dir->inode = inode;
	dir->buffer = NULL;
	dir->count = 0;
	dir->current = 0;
	dir->eof = false;
	return dir;
}

/**
 * 关闭目录
 */
void dir_close(DIR *dir){
	if(dir->inode->i_mounted){
		return;
	}
	inode_close(dir->inode);
	if(dir->buffer){
		kfree(dir);
	}
	kfree(dir);
}

bool delete_dir_entry(struct fext_inode_m *inode, uint32_t i_no);

/**
 * 删除磁盘上目录
 * @param par_i 父目录
 */
int32_t dir_remove(struct fext_inode_m *par_i, uint32_t i_no){
	if(!delete_dir_entry(par_i, i_no))
		return -1;
	inode_delete(par_i->fs, i_no);
	return 0;
}

/**
 * 初始化目录项
 */
void init_dir_entry(char *filename, uint32_t i_no, uint32_t f_type,\
		struct fext_dirent *dir_e){

	memset(dir_e, 0, sizeof(struct fext_dirent));
	memcpy(dir_e->filename, filename, MAX_FILE_NAME_LEN);
	dir_e->i_no = i_no;
	dir_e->f_type = f_type;
}

/**
 * 根据文件名搜索目录
 * @param m_inode 待搜索的目录
 * @param name 文件或目录名
 * @param dir_e 返回的目录项
 *
 * @return 搜索结果
 * 	@retval false 失败
 */
bool _search_dir_entry(struct fext_inode_m *m_inode, const char *name, 
		struct fext_dirent *dir_e){

	struct fext_fs *fs = m_inode->fs;
	uint32_t block_size = fs->sb->block_size;
	uint32_t per_block = block_size / sizeof(struct fext_dirent);
	uint32_t idx = 0;
	struct buffer_head *bh = NULL;
	uint32_t blk_nr;
	//挂载了别的分区，换成新inode
	if(m_inode->i_mounted){
		//TODO
		m_inode = fs->root_i;
	}	

	while((blk_nr = get_block_num(m_inode, idx, M_SEARCH))){

		bh = read_block(fs, blk_nr);
		uint32_t dir_entry_idx = 0;
		struct fext_dirent *p_de = (struct fext_dirent *)bh->data;

		while(dir_entry_idx < per_block){
			if(!strcmp(name, p_de->filename)){
				memcpy(dir_e, p_de, sizeof(struct fext_dirent));
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
 * 根据inode号在目录中查找目录项
 * @param m_inode 待搜索的目录
 * @param i_no inode号
 * @param dir_e 输出目录项
 * @return 是否成功
 * @note 这个函数和_search_dir_entry基本是一样的，但是从接口写成一个却有点麻烦
 */
bool search_dir_by_ino(struct fext_inode_m *m_inode, uint32_t i_no, 
		struct fext_dirent *dir_e){

	uint32_t block_size = m_inode->fs->sb->block_size;
	uint32_t per_block = block_size / sizeof(struct fext_dirent);
	uint32_t idx = 0;
	struct buffer_head *bh = NULL;
	uint32_t blk_nr;
	
	while((blk_nr = get_block_num(m_inode, idx, M_SEARCH))){
		bh = read_block(m_inode->fs, blk_nr);
		uint32_t dir_entry_idx = 0;
		struct fext_dirent *p_de = (struct fext_dirent *)bh->data;
		while(dir_entry_idx < per_block){
			if(p_de->i_no == i_no){
				memcpy(dir_e, p_de, sizeof(struct fext_dirent));
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
struct fext_inode_m *get_last_dir(const char *path){
	
	//根目录直接返回
	if(!strcmp(path, "/") || !strcmp(path, "/.") || !strcmp(path, "/..")){
		return root_fs->root_i;
	}

	char name[MAX_FILE_NAME_LEN];
	struct fext_inode_m *par_i = root_fs->root_i;
	struct fext_dirent dir_e;
	struct fext_fs *fs = root_fs;
	
	while((path = path_parse(path, name))){
		if(_search_dir_entry(par_i, name, &dir_e)){

			fs = par_i->fs;
			inode_close(par_i);

			if(!S_ISDIR(dir_e.f_type)){
				printk("%s is not a directory!\n", name);
				return NULL;
			}
			par_i = inode_open(fs, dir_e.i_no);
		}
		else{
			printk("%s is not found!\n", name);
			return NULL;
		}
	}
	return par_i;
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
struct fext_inode_m *search_dir_entry(const char *path, struct fext_dirent *dir_e){

	char filename[MAX_FILE_NAME_LEN];
	char dirname[MAX_PATH_LEN];

	split_path(path, filename, dirname);
	struct fext_inode_m *par_i = get_last_dir(dirname);

	//父目录不存在
	if(!par_i){
		printk("search_dir_entry: open directory error\n");
		return NULL;
	}

	//先查找
	bool found = _search_dir_entry(par_i, filename, dir_e);
	if(!found){
		inode_close(par_i);
		return NULL;
	}
	return par_i;
}

/**
 * @brief 在目录中添加目录项
 *
 * @param inode 指定目录指针
 * @param dir_e 目录项指针
 *
 * @reture 是否成功
 */
bool add_dir_entry(struct fext_inode_m *inode, struct fext_dirent *dir_e){

	struct fext_fs *fs = inode->fs;
	uint32_t block_size = fs->sb->block_size;
	struct buffer_head *bh = NULL;

	uint32_t blk_idx = inode->i_blocks ? 
		inode->i_blocks - 1 : 0;

	uint32_t off_byte = inode->i_size % block_size;

	if(blk_idx >= fs->max_blocks)
		return false;
	//块内没有剩余，这里假定目录项不跨块
	if(!off_byte && blk_idx){
		++blk_idx;
		inode->i_blocks += 1;
		if(blk_idx >= fs->max_blocks)
			return false;
	}
	
	//printk("blk_idx %d\n", blk_idx);
	uint32_t blk_nr = get_block_num(inode, blk_idx, M_CREATE);
	bh = read_block(fs, blk_nr);

	memcpy((bh->data + off_byte), dir_e, sizeof(struct fext_dirent));
	inode->i_size += sizeof(struct fext_dirent);
//	printk("%d\n", inode->i_size);
	write_block(fs, bh);
	release_block(bh);
	return true;
}


/**
 * 删除目录项
 *
 * @param inode 指定目录指针
 * @param i_no 待删除的目录项的inode号
 *
 * @note 目录项是定长设计，每次删除时会把最后一个补到当前空位上
 * @return 删除是否成功
 */
bool delete_dir_entry(struct fext_inode_m *inode, uint32_t i_no){

	struct fext_fs *fs = inode->fs;
	uint32_t block_size = fs->sb->block_size;
	uint32_t per_block = block_size / sizeof(struct fext_dirent);
	uint32_t idx = 0;
	struct buffer_head *bh = NULL;
	uint32_t blk_nr = 0;;
	uint32_t off_byte = 0;
	bool hit = false;

//先找出要删除的目录项
	while((blk_nr = get_block_num(inode, idx, M_SEARCH))){
		bh = read_block(fs, blk_nr);
		uint32_t dir_entry_idx = 0;
		struct fext_dirent *p_de = (struct fext_dirent *)bh->data;
		while(dir_entry_idx < per_block){
			if(i_no == p_de->i_no){
				hit = true;
				off_byte = dir_entry_idx * sizeof(struct fext_dirent);
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
	int fin_idx = inode->i_blocks - 1;
	//算错了。。。 先减再求模
	int fin_off_byte = (inode->i_size - sizeof(struct fext_dirent)) % block_size;
	struct buffer_head *fin_bh = NULL;

//	printk("fin_off_byte %d\n", fin_off_byte);
//	printk("off_byte %d\n", off_byte);

	//位于同一块内
	if(fin_idx == idx){
	
		fin_bh = bh;	
	}else {
		fin_bh = read_block(fs, get_block_num(inode, fin_idx, M_SEARCH));
	}

	memcpy((bh->data + off_byte), (fin_bh->data + fin_off_byte), sizeof(struct fext_dirent));

	write_block(fs, bh);
	release_block(bh);
	if(fin_idx != idx){
		write_block(fs, fin_bh);
		release_block(fin_bh);
	}

//检查是否有块已经空，如果有则调用remove_last释放该块
	inode->i_size -= sizeof(struct fext_dirent);
	inode->i_blocks = DIV_ROUND_UP(inode->i_size, block_size);
	if(!(inode->i_size % block_size))
		remove_last(inode);
	return true;
}

//===============================================================
//下面为相关系统调用执行函数
//===============================================================

/**
 * 删除空目录
 */
int32_t sys_rmdir(char *path){

	struct fext_dirent dir_e;
	struct fext_inode_m *par_i = search_dir_entry(path, &dir_e);

	//查找为空
	if(!par_i){
		printk("%s isn't exist\n", path);
		return -1;
	}
	
	//不是目录
	if(!S_ISDIR(dir_e.f_type)){
		printk("%s is not a directory\n", path);
		inode_close(par_i);
		return -1;
	}

	//目录不是空的
	struct fext_inode_m *inode = inode_open(par_i->fs, dir_e.i_no);
	if(!inode_is_empty(inode)){

		printk("dir %s is not empty, it is not allowd to delete\n", path);
		inode_close(par_i);
		inode_close(inode);
		return -1;
	}

	int32_t retval = dir_remove(par_i, inode->i_no);
	inode_close(par_i);
	inode_close(inode);
	return retval;
}

/**
 * 转换目录项
 */
static void swap_dirent(void *_dest, void *_src, uint32_t len){

	struct fext_dirent *src = _src;
	struct dirent *dest = _dest;
	uint32_t cnt = len / sizeof(struct fext_dirent);
	for(; cnt--; src++, dest++){
		strcpy(dest->d_name, src->filename);
		dest->d_ino = src->i_no;
	}
}
	
/**
 * 读取目录
 */
struct dirent *sys_readdir(DIR *dir){
	ASSERT(dir != NULL);
	struct fext_inode_m *inode = dir->inode;
	//第一次读
	if(dir->buffer == NULL){

		uint32_t block_size = root_fs->sb->block_size;
		uint32_t batch_size = block_size / 
			sizeof(struct fext_dirent) *
			sizeof(struct dirent);

		//这里直接以块为单位分配
		uint8_t *buffer = kmalloc(inode->i_blocks * block_size);
		//memset(buffer, 0, inode->i_blocks * block_size);
		uint32_t idx = 0;
		uint32_t blk_nr;
		struct buffer_head *bh = NULL;
		
		//全部读出来
		for(; (blk_nr = get_block_num(inode, idx, M_SEARCH)) ; idx++){
			bh = read_block(inode->fs, blk_nr);
			swap_dirent(buffer + idx * batch_size, bh->data, block_size);
			release_block(bh);
		}

		dir->count = inode->i_size / sizeof(struct fext_dirent);
		dir->buffer = (struct dirent *)buffer;
	}
	if(dir->current == dir->count){
		dir->eof = true;
		return NULL;
	}
	return dir->buffer + dir->current++;
}

/**
 * 复位目录偏移
 */
void sys_rewinddir(DIR *dir){
	dir->current = 0;
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
	struct fext_dirent dir_e;
	char filename[MAX_FILE_NAME_LEN];
	char dirname[MAX_PATH_LEN];

	split_path(path, filename, dirname);
	struct fext_inode_m *par_i = get_last_dir(dirname);

	//父目录不存在
	if(!par_i){
		printk("search_dir_entry: open directory error\n");
		return -1;
	}

	//先查找
	bool found = _search_dir_entry(par_i, filename, &dir_e);

	if(found){
		printk("sys_mkdir: file or directory %s exist!\n", path);
		inode_close(par_i);
		return -1;
	}

//申请inode
	struct fext_inode_m *m_inode = NULL;
	if(!(m_inode = inode_alloc(root_fs))){

		printk("sys_mkdir: kmalloc for inode failed\n");
		inode_close(par_i);
		return -1;
	}
	uint32_t i_no = m_inode->i_no;


//在父目录中添加目录项
	init_dir_entry(filename, i_no, S_IFDIR, &dir_e);
	if(!add_dir_entry(par_i, &dir_e)){
		printk("sys_mkdir: add_dir_entry failed\n");
		inode_close(par_i);
		inode_release(m_inode);
		return -1;
	}

//先磁盘同步两个inode
	inode_sync(par_i);
	inode_sync(m_inode);
	inode_release(m_inode);

	//再打开当前目录
	struct fext_inode_m *cur_i = inode_open(par_i->fs, i_no);

//TODO 考虑磁盘同步问题
	//添加目录项
	init_dir_entry(".",  i_no, S_IFDIR, &dir_e);
	add_dir_entry(cur_i, &dir_e);
	init_dir_entry("..", par_i->i_no, S_IFDIR, &dir_e);
	add_dir_entry(cur_i, &dir_e);

	//同步inode
	inode_sync(cur_i);
	inode_close(par_i);
	inode_close(cur_i);
//	printk("sys_mkdir done\n");
	return 0;
}

/**
 * 关闭目录
 */
int32_t sys_closedir(DIR *dir){

	if(!dir)
		return -1;
	dir_close(dir);
	return 0;
}

/**
 * 打开目录
 */
DIR *sys_opendir(char *path){

	if(path[0] == '/' && path[1] == 0){
		return dir_new(root_fs->root_i);
	}

	FIXUP_PATH(path);

	//先查找
	struct fext_dirent dir_e;
	struct fext_inode_m *par_i = search_dir_entry(path, &dir_e);

	if(par_i){
		inode_close(par_i);
		if(!S_ISDIR(dir_e.f_type)){
			printk("%s is a regular file\n", path);
			return NULL;
		}
		else{
			return dir_open(par_i->fs, dir_e.i_no);
		}
	}

	printk("%s isn't exist\n", path);
	return NULL;
}	

