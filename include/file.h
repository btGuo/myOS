#ifndef __FS_FILE_H
#define __FS_FILE_H

#include "global.h"
#include "stdint.h"

#define MAX_FILE_OPEN 12

struct file{
	uint32_t fd_pos;    ///< 文件偏移
	uint32_t fd_flag;   ///< 文件属性
	struct inode_info *fd_inode;   ///< 对应的i节点
};

/**
 * 标准输入输出
 */
enum std_fd{
	stdin_no,    ///< 0标准输入
	stdout_no,   ///< 1标准输出
	stderr_no    ///< 2标准错误
};

#endif
