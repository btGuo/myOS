#ifndef LIB_STRING_H
#define LIB_STRING_H
#include"stdint.h"
#include"global.h"

void memset(void *dest, uint8_t value, uint32_t size);
void memcpy(void *dest, void *src, uint32_t size);
int memcmp(const void *a, const void *b, uint32_t size);
char *strcpy(char *dest, const char *src);
uint32_t strlen(const char *str);
int8_t strcmp(const char *str1, const char *str2);
char *strchr(const char *str, const uint8_t ch);
char *strrchr(const char *str, const uint8_t ch);
char *strcat(char *dest, char *src);
uint32_t strchrs(const char *str, uint8_t ch);

#endif
