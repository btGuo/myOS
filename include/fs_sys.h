#ifndef FS_SYS_H
#define FS_SYS_H

#include "stdint.h"
#include "dir.h"
#include "fs.h"

int32_t sys_open(const char *path, uint8_t flags);
int32_t sys_close(int32_t fd);
int32_t sys_read(int32_t fd, void *buf, uint32_t count);
int32_t sys_write(int32_t fd, const void *buf, uint32_t count);
int32_t sys_lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t sys_unlink(const char *path);


int32_t sys_rmdir(char *path);
struct dirent *sys_readdir(struct Dir *dir);
void sys_rewinddir(struct Dir *dir);
int32_t sys_mkdir(char *path);
int32_t sys_closedir(struct Dir *dir);
struct Dir *sys_opendir(char *path);
int32_t sys_stat(const char *path, struct stat *st);
char *sys_getcwd(char *buf, uint32_t size);
int32_t sys_chdir(const char *path);

#endif
