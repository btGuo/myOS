#ifndef __FS_FILE_H
#define __FS_FILE_H

#include "global.h"
#include "stdint.h"

#define MAX_FILE_OPEN 12

struct file{
	uint32_t fd_pos;    ///< 文件偏移
	uint32_t fd_flag;   ///< 文件属性
	struct fext_inode_m *fd_inode;   ///< 对应的i节点
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

uint32_t to_global_fd(uint32_t fd);
int32_t get_fd();
int32_t set_fd(int32_t fd);
int32_t file_open(uint32_t i_no, uint8_t flag);
void file_close(struct file *file);
int32_t file_read(struct file *file, void *buf, uint32_t count);
int32_t file_write(struct file *file, const void *buf, uint32_t count);
#endif
