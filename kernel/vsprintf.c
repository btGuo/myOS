#include"print.h"
#include"stdarg.h"
#include"string.h"

static char toChar[17] = { '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

//数字转字符串, 可选十进制和十六进制
void to_num(int num, char *dest, char mode){
	char _tmp[1024];
	//这里多加一, 后面while舒服点
	char *tmp = _tmp + 1;
	if(mode == 'd'){
		while(num){
			*tmp++ = toChar[num % 10];
			num /= 10;
		}
	}else if(mode == 'h'){
		//16进制会在前面加0x
		*dest++ = '0';
		*dest++ = 'x';
		while(num){
			*tmp++ = toChar[num % 16];
			num /= 16;
		}
	}
	--tmp;
	while(tmp != _tmp){
		*dest++ = *tmp--;
	}
	*dest = '\0';
}
//简单到不能再简单的格式化字符串 @_@
//debug
void vsprintf(char *buf, const char *fmt, va_list args){
	char *dest = buf;
	while(*fmt){
		if(*fmt != '%'){
			*dest++ = *fmt++;
			*dest = '\0';
		}else{
			++fmt;
			if(*fmt == 's'){
				char *str = va_arg(args, char*);	
				while(*dest++ = *str++);
			}else if(*fmt == 'd' || *fmt == 'h'){
				int num = va_arg(args, int);
				to_num(num, dest, *fmt);
				while(*dest++);
			}
			--dest;
			++fmt;
		}
	}
}

