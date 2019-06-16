#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H

#include "stdint.h"

uint32_t getpid(void);
uint32_t write(int32_t fd, char *str, uint32_t count);
void* malloc(uint32_t size);
void free(void *ptr); 

void sys_call_init();

#endif 
