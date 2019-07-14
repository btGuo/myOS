#ifndef FS_SYS_H
#define FS_SYS_H

#include "stdint.h"
#include "dir.h"

int32_t sys_open(const char *path, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_read(int32_t fd, void *buf, uint32_t count);
int32_t sys_write(int32_t fd, const void *buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char *path);


int32_t sys_rmdir(char *path);
int32_t sys_readdir(struct dir *dir, struct dir_entry *dir_e);
void sys_rewinddir(struct dir *dir);
int32_t sys_mkdir(char *path);
int32_t sys_closedir(struct dir *dir);
struct dir *sys_opendir(char *path);

#endif
