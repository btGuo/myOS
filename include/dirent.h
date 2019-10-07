#ifndef __DIRENT_H
#define __DIRENT_H

#include <stdint.h>
#include <sys/types.h>
#include <stdbool.h>
#define DIRENT_NAME_LEN 24

/** posix */

/**
 * 目录，用于内存
 */
typedef struct _DIR{
	struct fext_inode_m *inode; ///< 对应的inode
	struct dirent *buffer; ///< 缓冲区
	uint32_t current;    ///< 当前遍历到的位置
	uint32_t count;      ///< 目录项总数
	bool     eof;        ///< 是否读完
}DIR;


struct dirent {
	char  d_name[DIRENT_NAME_LEN];
	ino_t d_ino;
};

struct dirent *readdir(DIR *dir);
void           rewinddir(DIR *dir);
int            closedir(DIR *dir);
DIR    *opendir(const char *path);

#endif
