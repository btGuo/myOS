#ifndef __DIRENT_H
#define __DIRENT_H

#include <stdint.h>
#include <global.h>
#define DIRENT_NAME_LEN 24

/** posix */

/**
 * 目录，用于内存
 */
struct Dir{
	struct inode_info *inode; ///< 对应的inode
	struct dirent *buffer; ///< 缓冲区
	uint32_t current;    ///< 当前遍历到的位置
	uint32_t count;      ///< 目录项总数
	bool     eof;        ///< 是否读完
};


struct dirent {
	char filename[DIRENT_NAME_LEN];
	uint32_t i_no;
};

struct dirent *readdir(struct Dir *dir);
void rewinddir(struct Dir *dir);
int32_t closedir(struct Dir *dir);
struct Dir *opendir(char *path);

#endif
