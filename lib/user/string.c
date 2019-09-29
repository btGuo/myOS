#include <string.h>

/**
 * 编译两个版本，内核和用户
 */
#ifdef __LIB_USER
	#include <assert.h>
	#include <stdio.h>
	#define ASSERT assert
#elif __LIB_KERNEL
	#include <debug.h>
	#include <global.h>
#endif


void *memset(void *dest, int value, size_t size){

	ASSERT(dest != NULL && size >= 0);
	int *tmp = (int*)dest;
	while(size--){
		*tmp++ = value;
	}
	return dest;
}

void *memcpy(void *dest, const void *src, size_t size){
	ASSERT(dest != NULL && src != NULL && size >= 0);
	int *_dest = (int*)dest;
	const int *_src = (int*)src;
	while(size--)
		*_dest++ = *_src++;
	return dest;
}

int memcmp(const void *a, const void *b, size_t size){
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

size_t strlen(const char *str){
	ASSERT(str != NULL);
	const char *_str = str;
	while(*_str++);
	return _str - str - 1;
}

int strcmp(const char *str1, const char *str2){
	ASSERT(str1 != NULL && str2 != NULL);
	while(*str1 != 0 && *str1 == *str2)
		++str1, ++str2;

	return *str1 < *str2 ? -1 : *str1 > *str2;
}

char *strchr(const char *str, const int ch){
	ASSERT(str != NULL);
	while(*str){
		if(*str == ch)
			return (char*)str;
		++str;
	}
	return NULL;
}

char *strrchr(const char *str, const int ch){
	ASSERT(str != NULL);
	const char *last_ch = NULL;
	while(*str){
		if(*str == ch)
			last_ch = str;
		++str;
	}
	return (char*)last_ch;
}

char *strcat(char *dest, const char *src){
	ASSERT(dest != NULL && src != NULL);
	char *tmp = dest;
	while(*tmp)
		++tmp;
	while((*tmp++ = *src++));
	return tmp;
}

size_t strchrs(const char *str, int ch){
	ASSERT(str != NULL);
	size_t cnt = 0;
	const char *_str = str;
	while(*_str){
		if(*_str == ch)
			++cnt;
		++_str;
	}
	return cnt;
}

char *strtok_r(char *str, const char *delim, char **saveptr){

        ASSERT(delim != NULL && saveptr != NULL);

        if(str){
                while(*str && strchr(delim, *str))str++;
        }
        if(str == NULL){
                str = *saveptr;
        }
        if(str == NULL){
                return NULL;
        }
        char *head = str;
        while(*str){

                if(strchr(delim, *str)){
                        *str++ = '\0';
                        while(*str && strchr(delim, *str))
                                str++;
                        if(*str == '\0')
                                *saveptr = NULL;
                        else
                                *saveptr = str;
                        return head;
                }
                str++;
        }
        *saveptr = NULL;
        return head;
}

			
