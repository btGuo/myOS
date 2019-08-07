#include "file.h"
#include "ide.h"
#include "super_block.h"
#include "thread.h"
#include "fs.h"
#include "interrupt.h"
#include "dir.h"
#include "string.h"
#include "block.h"
#include "pathparse.h"


struct file file_table[MAX_FILE_OPEN];   ///< 文件表
extern struct partition *cur_par;

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
 * 创建文件
 *
 * @param par_dir 新建文件的父目录
 * @param filename 文件名
 * @param flag 文件标志位
 *
 * @return 文件的inode号
 */
int32_t file_create(struct dir *par_dir, char *filename, uint8_t flag){

	//先分配inode
	struct inode_info *m_inode = NULL;
	if(!(m_inode = inode_alloc(cur_par))){
		//内存分配失败，回滚
		printk("file_create: kmalloc for inode failed\n");
		return -1;
	}

	uint32_t i_no = m_inode->i_no;
	//申请并安装目录项
	struct dir_entry dir_entry;
	init_dir_entry(filename, m_inode->i_no, FT_REGULAR, &dir_entry);
	if(!add_dir_entry(par_dir, &dir_entry)){
		printk("add_dir_entry: sync dir_entry to disk failed\n");
		inode_release(m_inode);
		return -1;
	}

	//磁盘同步两个inode
	inode_sync(cur_par, par_dir->inode);
	inode_sync(cur_par, m_inode);
	//记得释放
	inode_release(m_inode);

	return i_no;
}

/**
 * 打开文件
 * @param i_no 文件的inode号
 * @param flag 文件打开标志
 *
 * @return 是否成功
 * 	@retval -1 失败
 */
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
		//这里应该改为锁
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

/**
 * 关闭文件
 */
void file_close(struct file *file){

	inode_close(file->fd_inode);
	file->fd_inode = NULL;
}

/**
 * 写文件
 */
int32_t file_write(struct file *file, const void *buf, uint32_t count){

	struct buffer_head *bh = NULL;
	struct inode_info *m_inode = file->fd_inode;

	ASSERT(file->fd_pos <= m_inode->i_size);

	int32_t blk_idx = file->fd_pos / BLOCK_SIZE;
	int32_t off_byte = file->fd_pos % BLOCK_SIZE;
	uint8_t *src = (uint8_t *)buf;

	if(blk_idx >= BLOCK_LEVEL_3)
		return -1;

	uint32_t res = count;
	uint32_t to_write = 0;
	uint32_t blk_nr = 0;


	while(res){
		if(res > BLOCK_SIZE - off_byte){
			to_write = BLOCK_SIZE - off_byte;
			res -= to_write;
		}else {
			to_write = res;
			res = 0;
		}
		
		blk_nr = get_block_num(cur_par, m_inode, blk_idx, M_CREATE);
		bh = read_block(cur_par, blk_nr);
		memcpy((bh->data + off_byte), src, to_write);
	       	write_block(cur_par, bh);	
		release_block(bh);

		src += to_write;
		//第一次循环时有用
		off_byte = 0;
		++blk_idx;
		if(blk_idx >= BLOCK_LEVEL_3)
			return count - res;
	}
	//大于原来长度时才更新
	if(file->fd_pos + count > m_inode->i_size){
		m_inode->i_size = file->fd_pos + count;
		m_inode->i_blocks = DIV_ROUND_UP(m_inode->i_size, BLOCK_SIZE);
	}
//	printk("m_inode->i_size %d\n", m_inode->i_size);
	inode_sync(cur_par, m_inode);
	file->fd_pos += count;
	return count;
}

/**
 * 文件读
 *
 * @param file 文件描述符指针
 * @param buf 输出缓冲区
 * @param count 要读字节数
 *
 * @return 读取字节数
 */
int32_t file_read(struct file *file, void *buf, uint32_t count){

	struct buffer_head *bh = NULL;
	struct inode_info *m_inode = file->fd_inode;

	ASSERT(file->fd_pos + count <= m_inode->i_size);

	int32_t blk_idx = file->fd_pos / BLOCK_SIZE;
	int32_t off_byte = file->fd_pos % BLOCK_SIZE;
	uint8_t *dest = (uint8_t *)buf;

	uint32_t res = count;
	uint32_t to_read = 0;
	uint32_t blk_nr = 0;

	while(res){
		if(res > BLOCK_SIZE - off_byte){
			to_read = BLOCK_SIZE - off_byte;
			res -= to_read;
		}else {
			to_read = res;
			res = 0;
		}

		blk_nr = get_block_num(cur_par, m_inode, blk_idx, M_CREATE);
		bh = read_block(cur_par, blk_nr);
		memcpy(dest, (bh->data + off_byte), to_read);
		release_block(bh);
		dest += to_read;

		off_byte = 0;
	}
	file->fd_pos += count;
	return count;
}

//==============================================================
//下面为相关系统调用执行函数
//==============================================================

#define MAX_PATH_LEN 128

/**
 * 打开文件
 * @param path 文件路径
 * @param flags 模式，创建或者只是打开
 *
 * @return 打开的文件号
 * 	@retval -1 失败
 */
int32_t sys_open(const char *path, uint8_t flags){
	
	if(path[strlen(path) - 1] == '/'){
		printk("can't open a directory %s\n", path);
		return -1;
	}

	ASSERT(flags <= 7);
	int32_t fd = -1;

	//创建或者打开文件都需要先搜索
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

	//先查找，这里需要更高的灵活度，因此用了_开头的
	bool found = _search_dir_entry(cur_par, par_dir, filename, &dir_e);


	if(!found && !(flags & O_CREAT)){

		printk("file %s isn't exist\n", filename);
		dir_close(par_dir);
		return -1;

	}else if(found && (flags & O_CREAT)){

		printk("file %s has already exist!\n", filename);
		dir_close(par_dir);
		return -1;
	}
	if(flags & O_CREAT){

		printk("create file %s\n", filename);
		//TODO 错误处理
		int32_t i_no = file_create(par_dir, filename, flags);
		printk("i_no %d\n", i_no);
		fd = file_open(i_no, flags);
		dir_close(par_dir);

	}else {
		fd = file_open(dir_e.i_no, flags);
		printk("open file fd %d name %s i_no %d\n", fd, dir_e.filename, dir_e.i_no);
	}
	return fd;
}	

static inline uint32_t to_global_fd(uint32_t fd){

	ASSERT(fd >= 2 && fd < MAX_FILES_OPEN_PER_PROC);
	int32_t g_fd = curr->fd_table[fd];
	ASSERT(g_fd >= 0 && g_fd < MAX_FILE_OPEN);
	return g_fd;
}

/**
 * 关闭文件
 */
int32_t sys_close(int32_t fd){

	uint32_t g_fd = to_global_fd(fd);
	file_close(&file_table[g_fd]);
	curr->fd_table[fd] = -1;

	return g_fd;
}

#define FD_LEGAL(fd)\
do{\
	if((fd) < 0 || (fd) >= MAX_FILES_OPEN_PER_PROC){\
		printk("sys write: fd error\n");\
		return -1;\
	}\
}while(0)


/**
 * 文件写
 */
int32_t sys_write(int32_t fd, const void *buf, uint32_t count){

	FD_LEGAL(fd);
	if(fd == stdout_no){
		//TODO
		console_write(buf);
		return count;
	}
	uint32_t g_fd = to_global_fd(fd);
	struct file *file = &file_table[g_fd];
	if(file->fd_flag & O_WRONLY || file->fd_flag & O_RDWR){

		return file_write(file, buf, count);
	}else {
		printk("sys_write flag error\n");
		return -1;
	}
}

/**
 * 文件读
 */
int32_t sys_read(int32_t fd, void *buf, uint32_t count){

	FD_LEGAL(fd);
	uint32_t g_fd = to_global_fd(fd);
	struct file *file = &file_table[g_fd];

	return file_read(file, buf, count);
}

/**
 * 移动文件指针
 *
 * @param fd 文件描述符
 * @param offset 相对偏移量，可以为负数
 * @param whence 
 */
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence){

	FD_LEGAL(fd);
	ASSERT(whence > 0 && whence < 4);
	struct file *file = &file_table[to_global_fd(fd)];
	uint32_t file_size = file->fd_inode->i_size;
	int32_t new_pos = 0;

	switch(whence){
		case SEEK_SET:
			new_pos = offset;
			break;
		case SEEK_CUR:
			new_pos = file->fd_pos + offset;
			break;
		case SEEK_END:
			new_pos = file_size + offset;
			break;
	}
	if(new_pos < 0 || new_pos > file_size)
		return -1;

	file->fd_pos = new_pos;
	return new_pos;
}

/**
 * 系统调用，删除文件
 */
int32_t sys_unlink(const char *path){

	//先查找
	struct dir_entry dir_e;
	struct dir *par_dir = search_dir_entry(cur_par, path, &dir_e);

	//查找失败
	if(!par_dir){
		printk("file %s not found!\n", path);
		return -1;
	}

	//类型不对
	if(dir_e.f_type == FT_DIRECTORY){
		printk("can't delete a directory with unlink");
		dir_close(par_dir);
		return -1;
	}

	//查找是否已经在打开文件列表中
	uint32_t f_idx = 0;
	while(f_idx < MAX_FILE_OPEN){
		if(file_table[f_idx].fd_inode != NULL &&\
			dir_e.i_no == file_table[f_idx].fd_inode->i_no){

			dir_close(par_dir);
			printk("file %s is in use, not allow to delete\n", path);
			return -1;
		}
		++f_idx;
	}

	delete_dir_entry(cur_par, par_dir, dir_e.i_no);
	inode_delete(cur_par, dir_e.i_no);
	dir_close(par_dir);
	return 0;
}

