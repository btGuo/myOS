#ifndef __KERNEL_IO_H
#define __KERNEL_IO_H

#include <stdarg.h>

int printk(const char *fmt, ...);
void panic_spin(char *filename, int line, const char *func, const char *condition);
int vsprintf(char *buf, const char *fmt, va_list);
int sprintf(char *buf, const char *fmt, ...);

#define PANIC(...) panic_spin(__FILE__, __LINE__, __func__, __VA_ARGS__)

#ifdef NDEBUG
	#define ASSERT(CONDITION) ((void)0)
#else
	#define ASSERT(CONDITION) \
	if(!(CONDITION)){ \
		PANIC(#CONDITION); \
	}
#endif

#endif
