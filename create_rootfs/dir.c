#include "bitmap.h"
#include "inode.h"
#include "dir.h"
#include "block.h"
#include "pathparse.h"
#include "super_block.h"
#include "local.h"
#include "stat.h"

#include <string.h>

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

	uint32_t per_block = BLOCK_SIZE / sizeof(struct fext_dirent);
	uint32_t idx = 0;
	struct buffer_head *bh = NULL;
	uint32_t blk_nr;
	//挂载了别的分区，换成新inode
	if(m_inode->i_mounted){
		//TODO
		m_inode = m_inode->fs->root_i;
	}	

	while((blk_nr = get_block_num(m_inode, idx, M_SEARCH))){

		bh = read_block(m_inode->fs, blk_nr);
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
	uint32_t blk_nr = get_block_num(inode, blk_idx, M_CREATE);
	bh = read_block(inode->fs, blk_nr);

	memcpy((bh->data + off_byte), dir_e, sizeof(struct fext_dirent));
	inode->i_size += sizeof(struct fext_dirent);
//	printk("%d\n", inode->i_size);
	write_block(inode->fs, bh);
	release_block(bh);
	return true;
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
