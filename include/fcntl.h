#ifndef __POSIX_FCNTL_H
#define __POSIX_FCNTL_H

#include <sys/types.h>

int creat(const char *, mode_t);
int open(const char *, int, ...);

#endif
