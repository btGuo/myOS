#include"print.h"
#include"stdarg.h"

static char toChar[17] = { '0', '1', '2', '3', '4', '5',
	'6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};

void to_num(int num, char *dest, char mode){
	char _tmp[1024];
	char *tmp = _tmp;
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
//简单到不能再简单的格式化字符串^_^
void vsprintf(char *buf, const char *fmt, va_list args){
	char *dest = buf;

	while(*fmt){
		if(*fmt != '%'){
			*dest++ = *fmt++;
		}else{
			++fmt;
			if(*fmt == 's'){
				char *str = va_arg(args, char*);	
				strcpy(dest, str);
			}else if(*fmt == 'd'){
				int num = va_arg(args, int);
				to_num(num, dest, 'd');
			}else if(*fmt == 'h'){
				int num = va_arg(args, int);
				to_num(num, dest, 'h');
			}
			++fmt;
		}
	}
	*dest = '\0';
	put_str(buf);
}

