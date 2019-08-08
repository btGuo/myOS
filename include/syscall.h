#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H

#include "stdint.h"
#include "fs_sys.h"

uint32_t getpid(void);
uint32_t write(int32_t fd, char *str, uint32_t count);
void* malloc(uint32_t size);
void free(void *ptr); 
int32_t fork();
int32_t execv(const char *path, const char **argv);

#endif 
