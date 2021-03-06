#ifndef __FS_FILE_H
#define __FS_FILE_H

#include "global.h"
#include "stdint.h"
#include <sys/types.h>

#define MAX_FILE_OPEN 12

struct file{
	union fu{
		uint32_t        fu_pos; ///< 文件偏移
		struct ioqueue *fu_que; ///< 管道队列
	}fu;
	uint32_t             fd_flag;   ///< 文件属性
	uint32_t             fd_count;  ///< 打开次数
	struct fext_inode_m *fd_inode;  ///< 对应的i节点

	#define fd_pos  fu.fu_pos
	#define fd_que  fu.fu_que
};

/**
 * 标准输入输出
 */
enum std_fd{
	stdin_no,    ///< 0标准输入
	stdout_no,   ///< 1标准输出
	stderr_no    ///< 2标准错误
};

extern struct file file_table[MAX_FILE_OPEN];

struct file *get_file();
int32_t get_fd();

int32_t file_open(uint32_t i_no, int32_t flag);
int32_t file_create(struct fext_inode_m *par_i, char *filename, uint32_t type, mode_t mode);
void file_close(struct file *file);
int32_t file_read(struct file *file, void *buf, uint32_t count);
int32_t file_write(struct file *file, const void *buf, uint32_t count);
#endif
