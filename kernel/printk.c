#include"print.h"
#include"stdarg.h"

static char buf[1024];
void printk(const char *fmt, ...){
	put_str("printk start\n");
	va_list args;
	
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	put_str(buf);
	put_str("printk done\n");
}

