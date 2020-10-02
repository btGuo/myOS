#ifndef __POSIX_STDLIB_H
#define __POSIX_STDLIB_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifndef NULL
#define NULL (void*)0
#endif

#define EXIT_FAILURE -1
#define EXIT_SUCCESS 0

int  atoi(const char *src);
void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);
void *calloc(size_t lens, size_t size);
void exit(int);
///夹带私货，这个不是要求的
ssize_t readline(char **buf, size_t *buflen, int fd);

#ifdef __cplusplus
}
#endif

#endif
