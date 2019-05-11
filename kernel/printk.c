#include "stdarg.h"
#include "console.h"

//debug
static char buf[1024];
void printk(const char *fmt, ...){
	va_list args;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	console_write(buf);
}

