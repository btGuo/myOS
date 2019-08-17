#ifndef __LIB_USER_ASSERT_H
#define __LIB_USER_ASSERT_H
void __assert_handler(char *, int, const char *, char *);

#define assert(x)\
if(!(x)){\
	__assert_handler(__FILE__, __LINE__, __func__, #x);\
}\

#endif
