#ifndef __MM_H
#define __MM_H

#include <memlib.h>
#include <stddef.h>

void mm_print();
int mm_init(void);
void *mm_alloc(size_t size);
void *mm_calloc(size_t lens, size_t size);
void *mm_realloc(void *bp, size_t size);
void mm_free(void *bp);

#endif
