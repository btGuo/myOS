#include <file.h>
#include <ide.h>
#include <super_block.h>
#include <thread.h>
#include <fs.h>
#include <interrupt.h>
#include <dir.h>
#include <string.h>
#include <block.h>
#include <pathparse.h>
#include <tty.h>
#include <pipe.h>
#include <char_dev.h>
#include <stdarg.h>


struct file file_table[MAX_FILE_OPEN];   ///< 文件表

int32_t sys_close(int32_t fd);

static int dupfd(uint32_t fd, uint32_t arg){
	
	if(fd < 0 || fd >= MAXL_OPENS ||
			curr->fd_table[fd] == NULL){

		return -1;
	}
	if(arg < 0 || arg >= MAXL_OPENS){

		return -1;
	}

	while(arg < MAXL_OPENS){

		if(curr->fd_table[arg] == NULL){
			
			break;
		}
		arg++;
	}

	if(arg == MAXL_OPENS){
		return -1;
	}

	(curr->fd_table[arg] = curr->fd_table[fd])->fd_count++;
	return arg;
}

int32_t sys_dup(int32_t fd){

	return dupfd(fd, 0);
}

int32_t sys_dup2(int32_t oldfd, int32_t newfd){

	sys_close(newfd);
	return dupfd(oldfd, newfd);
}
//TODO 修改相关调用

/**
 * @brief 在文件表中找到空位
 */
struct file *get_file(){

	uint32_t idx = 3;
	while(idx < MAX_FILE_OPEN){
		if(file_table[idx].fd_inode == NULL){
			return &file_table[idx];
		}
		++idx;
	}
	return NULL;
}

/**
 * 在当前进程中查找空闲项
 */
int32_t get_fd(){
	
	int32_t idx = 0;
	while(idx < MAXL_OPENS){

		if(curr->fd_table[idx] == NULL){

			return idx;
		}
		idx++;
	}
	return -1;
}

/**
 * 创建文件
 *
 * @param par_dir 新建文件的父目录
 * @param filename 文件名
 * @param type 文件类型
 * @param mode 访问权限
 *
 * @return 文件的inode号
 */
int32_t file_create(struct fext_inode_m *par_i, 
		char *filename, uint32_t type, mode_t mode){

	//先分配inode
	struct fext_inode_m *m_inode = NULL;
	if(!(m_inode = inode_alloc(par_i->fs))){
		//内存分配失败，回滚
		printk("file_create: kmalloc for inode failed\n");
		return -1;
	}

	uint32_t i_no = m_inode->i_no;
	//申请并安装目录项
	struct fext_dirent dir_entry;
	init_dir_entry(filename, m_inode->i_no, type, &dir_entry);
	if(!add_dir_entry(par_i, &dir_entry)){
		printk("add_dir_entry: sync dir_entry to disk failed\n");
		inode_release(m_inode);
		return -1;
	}
	
	//注意磁盘中是16位的
	m_inode->i_type = (uint16_t)type;
	m_inode->i_mode = (uint16_t)mode;

	if(curr){
		m_inode->i_uid = curr->uid;
		m_inode->i_gid = curr->gid;
	}else {
		m_inode->i_uid = 0;
		m_inode->i_gid = 0;
	}

	//磁盘同步两个inode
	inode_sync(par_i);
	inode_sync(m_inode);
	//记得释放
	inode_release(m_inode);

	return i_no;
}

//TODO debug
/**
 * 检查权限是否允许
 */
bool check_right(struct fext_inode_m *m_inode, int flags){

	uint16_t mode = m_inode->i_mode;
	mode &= 0777;
	if(curr->uid == m_inode->i_uid){

		mode >>= 6;

	}else if(curr->gid == m_inode->i_gid){

		mode &= 070;
		mode >>= 3;
	}else {
		mode &= 07;
	}

	//取出rw位
	mode  &= 06;
	flags &= 06;

	return (mode & flags);
}


/**
 * 打开文件
 * @param i_no 文件的inode号
 * @param flag 文件打开标志
 *
 * @return 是否成功
 * 	@retval -1 失败
 */
int32_t file_open(uint32_t i_no, int32_t flag){

	struct file *fp = get_file();
	int32_t fd = get_fd();
	int32_t ret = fd;

	if(fp == NULL || fd == -1){

	//达到全局最大文件打开数或者当前进程最大文件打开数
		return -1;
	}

	//注意设置这两项，标志已经用了
	curr->fd_table[fd] = fp;
	fp->fd_inode = inode_open(root_fs, i_no);

	if(fp->fd_inode == NULL){
		
		curr->fd_table[fd] = NULL;
		return -1;
	}

	if(!check_right(fp->fd_inode, flag)){
		
		printk("you have no right to open this file\n");
		ret = -1;
		goto error;
	}

	fp->fd_pos = 0;
	fp->fd_flag = flag;
	fp->fd_count = 1;

	bool *write_deny = &fp->fd_inode->i_write_deny;

	if(flag & O_WRONLY || flag & O_RDWR){
		//这里应该改为锁
		enum intr_status old_stat = intr_disable();
		if(!(*write_deny)){
			*write_deny = true;
			intr_set_status(old_stat);
		}else {
			intr_set_status(old_stat);
			printk("file can't be write now, try again later\n");
			
			ret = -1;
			goto error;
		}
	}
	return ret;

error:
	inode_close(fp->fd_inode);
	fp->fd_inode = NULL;
	curr->fd_table[fd] = NULL;

	return ret;
}

//TODO 
/**  inode 和 file 都有引用计数，有点重复了 */

/**
 * 关闭文件
 */
void file_close(struct file *file){

	if(--file->fd_count == 0){

		inode_close(file->fd_inode);
		file->fd_inode = NULL;
	}
}

/**
 * 写文件
 */
int32_t file_write(struct file *file, const void *buf, uint32_t count){

	struct buffer_head *bh = NULL;
	struct fext_inode_m *m_inode = file->fd_inode;
	struct fext_fs *fs = m_inode->fs;
	uint32_t block_size = fs->sb->block_size;

	ASSERT(file->fd_pos <= m_inode->i_size);

	int32_t blk_idx = file->fd_pos / block_size;
	int32_t off_byte = file->fd_pos % block_size;
	uint8_t *src = (uint8_t *)buf;

	if(blk_idx >= fs->max_blocks)
		return -1;

	uint32_t res = count;
	uint32_t to_write = 0;
	uint32_t blk_nr = 0;


	while(res){
		if(res > block_size - off_byte){
			to_write = block_size - off_byte;
			res -= to_write;
		}else {
			to_write = res;
			res = 0;
		}
		
		blk_nr = get_block_num(m_inode, blk_idx, M_CREATE);
		bh = read_block(fs, blk_nr);
		memcpy((bh->data + off_byte), src, to_write);
	       	write_block(fs, bh);	
		release_block(bh);

		src += to_write;
		//第一次循环时有用
		off_byte = 0;
		++blk_idx;
		if(blk_idx >= fs->max_blocks)
			return count - res;
	}
	//大于原来长度时才更新
	if(file->fd_pos + count > m_inode->i_size){
		m_inode->i_size = file->fd_pos + count;
		m_inode->i_blocks = DIV_ROUND_UP(m_inode->i_size, block_size);
	}
//	printk("m_inode->i_size %d\n", m_inode->i_size);
	inode_sync(m_inode);
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
	struct fext_inode_m *m_inode = file->fd_inode;
	struct fext_fs *fs = m_inode->fs;
	uint32_t block_size = fs->sb->block_size;

	if(file->fd_pos + count > m_inode->i_size){

		//读取剩下的字节
		count = m_inode->i_size - file->fd_pos;
	}

	int32_t blk_idx = file->fd_pos / block_size;
	int32_t off_byte = file->fd_pos % block_size;
	uint8_t *dest = (uint8_t *)buf;

	uint32_t res = count;
	uint32_t to_read = 0;
	uint32_t blk_nr = 0;

	while(res){
		if(res > block_size - off_byte){
			to_read = block_size - off_byte;
			res -= to_read;
		}else {
			to_read = res;
			res = 0;
		}

		blk_nr = get_block_num(m_inode, blk_idx, M_CREATE);
		bh = read_block(fs, blk_nr);
		memcpy(dest, (bh->data + off_byte), to_read);
		release_block(bh);
		dest += to_read;
		blk_idx++;

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
int sys_open(const char *path, int flags, mode_t mode){
	
	if(path[strlen(path) - 1] == '/'){
		printk("can't open a directory %s\n", path);
		return -1;
	}

	//ASSERT(flags <= 7);
	int32_t fd = -1;

	//创建或者打开文件都需要先搜索
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

	//先查找，这里需要更高的灵活度，因此用了_开头的
	bool found = _search_dir_entry(par_i, filename, &dir_e);


	if(!found && !(flags & O_CREAT)){

		printk("file %s isn't exist\n", filename);
		goto error;

	}else if(found && (flags & O_CREAT)){

		printk("file %s has already exist!\n", filename);
		goto error;
	}
	if(flags & O_CREAT){

		mode &= ~curr->umask;

		//printk("create file %s\n", filename);
		int32_t i_no = file_create(par_i, filename, S_IFREG, mode);

		if(i_no == -1){

			goto error;
		}

		fd = file_open(i_no, flags);

	}else {

		fd = file_open(dir_e.i_no, flags);
		printk("open file fd %d name %s i_no %d\n", 
				fd, dir_e.filename, dir_e.i_no);
	}

error:
	inode_close(par_i);
	return fd;
}	

/**
 * 关闭文件
 * @return 成功返回0，失败返回-1
 */
int32_t sys_close(int32_t fd){

	if(fd < 0 || fd >= MAXL_OPENS ||
		curr->fd_table[fd] == NULL){

		return -1;
	}

	uint32_t type = curr->fd_table[fd]->fd_inode->i_type;
	if(S_ISFIFO(type)){

		pipe_close(fd);

	}else if(S_ISREG(type)){

		file_close(curr->fd_table[fd]);
	}
	curr->fd_table[fd] = NULL;
	return 0;
}

/**
 * 文件写
 */
int32_t sys_write(int32_t fd, const void *buf, uint32_t count){

	if(fd < 0 || fd >= MAXL_OPENS ||
		curr->fd_table[fd] == NULL){

		return -1;
	}
	//printk("sys_write\n");

	struct file *fp = curr->fd_table[fd];
	struct fext_inode_m *inode = fp->fd_inode;
	//printk("i_type %x\n", inode->i_type);


	if(S_ISREG(inode->i_type)){

		if(fp->fd_flag & O_WRONLY || fp->fd_flag & O_RDWR){

			return file_write(fp, buf, count);
		}
		printk("sys_write flag error\n");
		return -1;
	}

	if(S_ISCHR(inode->i_type)){

		//printk("write char dev\n");
		//TODO 目前这里固定为0号
		chdev_writef writep = chdev_wrtlb[0];
		return writep(buf, count);

	}

	if(S_ISFIFO(inode->i_type)){

		//printk("write fifo\n");
		return pipe_write(fd, buf, count);
	}

	if(S_ISBLK(inode->i_type)){
		//TODO
	}
	return -1;
}

/**
 * 文件读
 */
int32_t sys_read(int32_t fd, void *buf, uint32_t count){

	if(fd < 0 || fd >= MAXL_OPENS ||
		curr->fd_table[fd] == NULL){

		printk("wrong fd\n");
		return -1;
	}

	struct file *fp = curr->fd_table[fd];
	struct fext_inode_m *inode = fp->fd_inode;

	if(S_ISREG(inode->i_type)){

		//printk("read regular\n");
		return file_read(fp, buf, count);
	}

	if(S_ISCHR(inode->i_type)){

		//TODO 同上
		chdev_readf readp = chdev_rdtlb[0];
		return readp(buf, count);
	}

	if(S_ISFIFO(inode->i_type)){

		return pipe_read(fd, buf, count);
	}

	if(S_ISBLK(inode->i_type)){
		//TODO
	}
	return -1;
}

/**
 * 移动文件指针
 *
 * @param fd 文件描述符
 * @param offset 相对偏移量，可以为负数
 * @param whence 
 */
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence){

	if(fd < 0 || fd >= MAXL_OPENS ||
		curr->fd_table[fd] == NULL){

		return -1;
	}
	if(whence <= 0 || whence >= 4){

		return -1;
	}

	struct file *file = curr->fd_table[fd];
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
	struct fext_dirent dir_e;
	struct fext_inode_m *par_i = search_dir_entry(path, &dir_e);

	//查找失败
	if(!par_i){
		printk("file %s not found!\n", path);
		return -1;
	}

	//类型不对
	if(!S_ISREG(dir_e.f_type)){
		printk("can't delete a directory with unlink");
		inode_close(par_i);
		return -1;
	}

	//查找是否已经在打开文件列表中
	uint32_t f_idx = 0;
	while(f_idx < MAX_FILE_OPEN){
		if(file_table[f_idx].fd_inode != NULL &&\
			dir_e.i_no == file_table[f_idx].fd_inode->i_no){

			inode_close(par_i);
			printk("file %s is in use, not allow to delete\n", path);
			return -1;
		}
		++f_idx;
	}

	delete_dir_entry(par_i, dir_e.i_no);
	inode_delete(par_i->fs, dir_e.i_no);
	inode_close(par_i);
	return 0;
}

int sys_chmod(const char *filename, mode_t mode){

	struct fext_inode_m *inode = path2inode(filename);

	if(inode == NULL){
		return -1;
	}
	//不是文件所有者同时也不是超级用户
	if(inode->i_uid != curr->uid && !is_super()){

		return -1;
	}
	inode->i_mode = (mode & 07777) | (inode->i_mode & ~07777);
	inode_sync(inode);
	inode_close(inode);
       		
	return 0;
}

