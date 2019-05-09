#ifndef __STDARG_H
#define __STDARG_H

typedef char *va_list;

//字节对齐
#define __va_round_size(type) \
	(((sizeof(type) + sizeof(int) - 1) / sizeof(int)) * sizeof(int))

#define va_start(p, arg) \
	(p = ((char*)&(arg) + __va_round_size(arg)))

void va_end(va_list);
#define va_end(p)

#define va_arg(p, type) \
	(p += __va_round_size(type), \
	 *((type*) (p - __va_round_size(type)))) 

#endif

