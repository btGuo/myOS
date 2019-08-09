#include "stdarg.h"
#include "tty.h"

//debug
void printk(const char *fmt, ...){
	va_list args;
	
	char buf[1024] = {0};
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	terminal_writestr(buf);
}

