#ifndef __POSIX_FCNTL_H
#define __POSIX_FCNTL_H

#include <sys/types.h>

#define O_RDONLY 4  ///< 只读 
#define O_WRONLY 2  ///< 只写 
#define O_RDWR   6  ///< 读写 
#define O_CREAT  8  ///< 创建 


int creat(const char *, mode_t);
int open(const char *, int, ...);

#endif
