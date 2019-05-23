#ifndef LIB_USER_SYSCALL_H
#define LIB_USER_SYSCALL_H

#include "stdint.h"

uint32_t getpid(void);
uint32_t write(char *str);
void* malloc(uint32_t size);

void sys_call_init();

#endif 
