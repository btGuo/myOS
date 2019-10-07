#include <fs.h>
#include <file.h>
#include <stdint.h>
#include <global.h>
#include <thread.h>
#include <ioqueue.h>
#include <inode.h>


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

	//只计算到i_type的大小
	uint32_t size = offsetof(struct fext_inode_m,  i_type) + 
			st_sizeof(struct fext_inode_m, i_type);

	// !!! 这里单独申请，注意inode只有一部分可用
	struct fext_inode_m *inode = kmalloc(size);
	if(inode == NULL){
		curr->fd_table[fd1] = NULL;
		curr->fd_table[fd2] = NULL;
		kfree(que);
		return -1;
	}

	//管道类型，只用到了一个成员
	inode->i_type = S_IFIFO;

	fp->fd_que = que;
	fp->fd_inode = inode;
	//可读写
	fp->fd_flag = O_RDWR;
	//打开次数已经是2了
	fp->fd_count = 2;

	return 0;
}

/**
 * 管道写
 * @param fd 文件描述符
 * @param _buf 输入缓冲区
 * @param cnt 写入字节数
 * @return 成功写入字节数
 */
uint32_t pipe_read(int32_t fd, void *buf, uint32_t cnt){

	struct file *fp = curr->fd_table[fd];
	ASSERT(fp);

	return queue_read(fp->fd_que, buf, cnt, IOQUEUE_NON_BLOCK);
}

/**
 * 管道读
 * @param fd 文件描述符
 * @param _buf 输出缓冲区
 * @param cnt 读取字节数
 * @return 成功读取字节数
 */
uint32_t pipe_write(int32_t fd, const void *buf, uint32_t cnt){

	struct file *fp = curr->fd_table[fd];
	ASSERT(fp);

	return queue_write(fp->fd_que, buf, cnt, IOQUEUE_NON_BLOCK);
}

void pipe_close(int32_t fd){

	struct file *fp = curr->fd_table[fd];
	ASSERT(fp);

	if(--fp->fd_count == 0){
		queue_release(fp->fd_que);
		kfree(fp->fd_inode);
		fp->fd_inode = NULL;
	}
}
