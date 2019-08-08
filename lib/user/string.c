#include "string.h"
#include "assert.h"
#include "print.h"

void memset(void *dest, uint8_t value, uint32_t size){
	ASSERT(dest != NULL && size >= 0);
	uint8_t *tmp = (uint8_t*)dest;
	while(size--){
		*tmp++ = value;
	}
}

void memcpy(void *dest, void *src, uint32_t size){
	ASSERT(dest != NULL && src != NULL && size >= 0);
	uint8_t *_dest = (uint8_t*)dest;
	const uint8_t *_src = (uint8_t*)src;
	while(size--)
		*_dest++ = *_src++;
}

int memcmp(const void *a, const void *b, uint32_t size){
	ASSERT(a != NULL && b != NULL && size >= 0);
	const char *_a = a;
	const char *_b = b;
	while(size--){
		if(*_a != *_b)
			return *_a > *_b ? 1 : -1;
		++_a;
		++_b;
	}
	return 0;
}

char *strcpy(char *dest, const char *src){
	ASSERT(dest != NULL && src != NULL);
	char *_dest = dest;
	while((*dest++ = *src++));
	return _dest;
}

uint32_t strlen(const char *str){
	ASSERT(str != NULL);
	const char *_str = str;
	while(*_str++);
	return _str - str - 1;
}

int8_t strcmp(const char *str1, const char *str2){
	ASSERT(str1 != NULL && str2 != NULL);
	while(*str1 != 0 && *str1 == *str2)
		++str1, ++str2;

	return *str1 < *str2 ? -1 : *str1 > *str2;
}

char *strchr(const char *str, const uint8_t ch){
	ASSERT(str != NULL);
	while(*str){
		if(*str == ch)
			return (char*)str;
		++str;
	}
	return NULL;
}

char *strrchr(const char *str, const uint8_t ch){
	ASSERT(str != NULL);
	const char *last_ch = NULL;
	while(*str){
		if(*str == ch)
			last_ch = str;
		++str;
	}
	return (char*)last_ch;
}

char *strcat(char *dest, char *src){
	ASSERT(dest != NULL && src != NULL);
	char *tmp = dest;
	while(*tmp)
		++tmp;
	while((*tmp++ = *src++));
	return tmp;
}

uint32_t strchrs(const char *str, uint8_t ch){
	ASSERT(str != NULL);
	uint32_t cnt = 0;
	const char *_str = str;
	while(*_str){
		if(*_str == ch)
			++cnt;
		++_str;
	}
	return cnt;
}
		
