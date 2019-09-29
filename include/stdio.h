#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include <stdarg.h>

int printf(const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list);
int sprintf(char *buf, const char *fmt, ...);

#endif
