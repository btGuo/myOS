#ifndef __LIB_KERNEL_PRINT_H
#define __LIB_KERNEL_PRINT_H
#include "stdint.h"
#include "stdarg.h"

void put_char(uint8_t asci);
void put_str(char *str);
void put_int(uint32_t num);
void printk(const char *fmt, ...);
void vsprintf(char *buf, const char *fmt, va_list);
#endif
