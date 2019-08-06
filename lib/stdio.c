#include "stdio.h"
#include "syscall.h"
#include "stdarg.h"
#include "string.h"

uint32_t printf(const char *fmt, ...){
	va_list args;
	
	char buf[1024] = {0};
	buf[100] = 'T';
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	//和printk区别在于系统调用
	return write(1, buf, strlen(buf));
}
