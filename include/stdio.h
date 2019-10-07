#ifndef __LIB_STDIO_H
#define __LIB_STDIO_H

#include <stdarg.h>
#include <sys/types.h>

typedef struct FILE
{
	int fd;
}FILE;

int printf(const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list);
int sprintf(char *buf, const char *fmt, ...);
int putchar(int);

FILE  *fopen(const char *, const char *);
size_t fread(void *, size_t, size_t, FILE *);
size_t fwrite(const void *, size_t, size_t, FILE *);
int    fclose(FILE *);

#endif
