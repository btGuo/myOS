#include "print.h"
#include "stdarg.h"

void sprintf(char *buf, const char *fmt, ...){
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
}

