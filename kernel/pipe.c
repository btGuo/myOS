#include <fs.h>
#include <file.h>
#include <stdint.h>
#include <global.h>
#include <thread.h>
#include <ioqueue.h>
#include <inode.h>


//TODO debug
/**
 * 创建管道
 */
int32_t sys_pipe(int32_t pipefd[2]){

	struct file *fp = get_file();
	if(fp == NULL){
		return -1;
	}

	int32_t fd1 = get_fd();
	if(fd1 == -1){

		return -1;
	}
	curr->fd_table[fd1] = fp;
	int32_t fd2 = get_fd();
	if(fd2 == -1){
		
		curr->fd_table[fd1] = NULL;
		return -1;
	}
	curr->fd_table[fd2] = fp;

	pipefd[0] = fd1;
	pipefd[1] = fd2;

	struct ioqueue *que = kmalloc(sizeof(struct ioqueue));
	if(!que){
		curr->fd_table[fd1] = NULL;
		curr->fd_table[fd2] = NULL;
		return -1;
	}

	ioqueue_init(que);

	//这里复用了fd_inode强制类型转换
	fp->fd_inode = (struct fext_inode_m *)que;
	fp->fd_flag = S_IFIFO;
	//复用fd_pos为管道打开数
	fp->fd_pos = 2;

	return 0;
}

/**
 * 管道写
 * @param fd 文件描述符
 * @param _buf 输入缓冲区
 * @param cnt 写入字节数
 * @return 成功写入字节数
 */
uint32_t pipe_read(int32_t fd, void *_buf, uint32_t cnt){

	char *buf = _buf;

	struct file *fp = curr->fd_table[fd];

	ASSERT(fp);

	struct ioqueue *que = (struct ioqueue *)fp->fd_inode;

	uint32_t qlen = queue_len(que);
	uint32_t bytes = cnt > qlen ? qlen : cnt;
	uint32_t ret = bytes;
	while(bytes--)
		*buf++ = queue_getchar(que);
	
	return ret;
}

/**
 * 管道读
 * @param fd 文件描述符
 * @param _buf 输出缓冲区
 * @param cnt 读取字节数
 * @return 成功读取字节数
 */
uint32_t pipe_write(int32_t fd, const void *_buf, uint32_t cnt){


	char *buf = _buf;

	struct file *fp = curr->fd_table[fd];

	ASSERT(fp);
	struct ioqueue *que = (struct ioqueue *)fp->fd_inode;

	uint32_t qres = que->size - queue_len(que);
	uint32_t bytes = cnt > qres ? qres : cnt;
	uint32_t ret = bytes;
	while(bytes--)
		queue_putchar(que, *buf++);

	return ret;
}

void pipe_close(int32_t fd){

	struct file *fp = curr->fd_table[fd];
	ASSERT(fp);

	if(--fp->fd_pos == 0){
		queue_release((struct ioqueue *)fp->fd_inode);
		fp->fd_inode = NULL;
	}
}


/*
void sys_fd_redirect(uint32_t old_fd, uint32_t new_fd){
	new_fd < 3 ?
		(curr->fd_table[old_fd] = new_fd):
		(curr->fd_table[old_fd] = curr->fd_table[new_fd]);
}
*/

