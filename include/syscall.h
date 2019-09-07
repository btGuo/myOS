#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H

#include <stdint.h>
#include <dir.h>
#include <fs.h>
#include <dirent.h>

uint32_t getpid(void);
void* malloc(uint32_t size);
void free(void *ptr); 
int32_t fork();
int32_t execv(const char *path, const char **argv);
void exit(int32_t status);
int32_t wait(int32_t status);
void clear();
void putchar(char ch);

int32_t open(const char *path, uint8_t flags);
int32_t close(int32_t fd);
int32_t read(int32_t fd, void *buf, uint32_t count);
int32_t write(int32_t fd, const void *buf, uint32_t count);
int32_t lseek(int32_t fd, int32_t offset, uint8_t whence);
int32_t unlink(const char *path);

int32_t rmdir(char *path);
int32_t mkdir(char *path);
int32_t stat(const char *path, struct stat *st);
#endif 
