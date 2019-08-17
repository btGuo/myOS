#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include "stdint.h"
#include "stdarg.h"

#define NULL (void*)0
#define bool char
#define true 1
#define false 0
#define DIV_ROUND_UP(x, step) (((x) + (step) - 1) / (step))

uint32_t printf(const char *fmt, ...);
void vsprintf(char *buf, const char *fmt, va_list);
void sprintf(char *buf, const char *fmt, ...);
int32_t atoi(char *src);
void itoa(int num, char *dest, char mode);
void str_align(char *str, char *dest, uint32_t width, bool left);

#endif
