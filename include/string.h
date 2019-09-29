#ifndef LIB_STRING_H
#define LIB_STRING_H

#include <stddef.h>

void *memset(void *dest, int value, size_t size);
void *memcpy(void *dest, const void *src, size_t size);
int memcmp(const void *a, const void *b, size_t size);

char *strcpy(char *dest, const char *src);
size_t strlen(const char *str);
int strcmp(const char *str1, const char *str2);

char *strchr(const char *str, int ch);
char *strrchr(const char *str, int ch);
char *strcat(char *dest, const char *src);
size_t strchrs(const char *str, int ch);

char *strtok_r(char *str, const char *delim, char **saveptr);

#endif
