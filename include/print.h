#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
#include "stdarg.h"

void printk(const char *fmt, ...);
void vsprintf(char *buf, const char *fmt, va_list);
void sprintf(char *buf, const char *fmt, ...);
#endif
