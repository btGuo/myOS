#ifndef _KERNEL_TTY_H
#define _KERNEL_TTY_H

#include "stdint.h"

void terminal_init(void);
void terminal_writestr(const char* data);
void terminal_clear();
void terminal_putchar(char c);
int32_t terminal_read(char *data, uint32_t size);
int32_t terminal_write(const char* data, uint32_t size);

#endif
