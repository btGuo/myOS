#include <fs.h>
#include <file.h>
#include <stdint.h>
#include <global.h>
#include <thread.h>
#include <ioqueue.h>
#include <inode.h>

/**
 * 判断文件类型是否是管道
 */
bool is_pipe(uint32_t l_fd){
	return file_table[to_global_fd(l_fd)].fd_flag == S_IFIFO;
}

/**
 * 创建管道
 */
int32_t sys_pipe(int32_t pipefd[2]){

	int32_t gfd = get_fd();
	struct ioqueue *que = kmalloc(sizeof(struct ioqueue));
	if(!que){
		printk("no more space \n");
		return -1;
	}

	ioqueue_init(que);
	//这里复用了fd_inode强制类型转换
	file_table[gfd].fd_inode = (struct fext_inode_m *)que;
	file_table[gfd].fd_flag = S_IFIFO;
	//复用fd_pos为管道打开数
	file_table[gfd].fd_pos = 2;

	pipefd[0] = set_fd(gfd);
	pipefd[1] = set_fd(gfd);

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

	int32_t gfd = to_global_fd(fd);
	ASSERT(file_table[gfd].fd_flag == S_IFIFO);
	struct ioqueue *que = (struct ioqueue *)file_table[gfd].fd_inode;

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
	int32_t gfd = to_global_fd(fd);
	ASSERT(file_table[gfd].fd_flag == S_IFIFO);
	struct ioqueue *que = (struct ioqueue *)file_table[gfd].fd_inode;

	uint32_t qres = que->size - queue_len(que);
	uint32_t bytes = cnt > qres ? qres : cnt;
	uint32_t ret = bytes;
	while(bytes--)
		queue_putchar(que, *buf++);

	return ret;
}

void pipe_close(int32_t fd){

	int32_t gfd = to_global_fd(fd);
	if(--file_table[gfd].fd_pos == 0){
		queue_release((struct ioqueue *)file_table[gfd].fd_inode);
		file_table[gfd].fd_inode = NULL;
	}
}


/**
 * 重定向文件描述符
 */
void sys_fd_redirect(uint32_t old_fd, uint32_t new_fd){
	new_fd < 3 ?
		(curr->fd_table[old_fd] = new_fd):
		(curr->fd_table[old_fd] = curr->fd_table[new_fd]);
}

