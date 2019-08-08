#include"stdarg.h"
#include"string.h"

static const char map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',\
	'9', 'a', 'b', 'c', 'd', 'e', 'f'};

//TODO 数为0时没有打印

//数字转字符串, 可选十进制和十六进制
void to_num(int num, char *dest, char mode){
	char _tmp[1024];
	//这里多加一, 后面while舒服点
	char *tmp = _tmp + 1;
	if(mode == 'd'){
		if(num < 0){
			num = -num;
			*dest++ = '-';
		}
		while(num){
			*tmp++ = map[num % 10];
			num /= 10;
		}
		--tmp;
		//反向复制过去
		while(tmp != _tmp){
			*dest++ = *tmp--;
		}

	}else if(mode == 'h'){
		//十六进制可以直接从字节入手
		//会在前面加0x
		*dest++ = '0';
		*dest++ = 'x';
		unsigned char *q = (unsigned char *)&num + 3;
		unsigned char byte;
		int n = 4;
		while(n--){
			byte = *q;
			*dest++ = map[byte >> 4];
			*dest++ = map[byte & 0x0f];
			--q;
		}
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
				while((*dest++ = *str++));
				--dest;

			}else if(*fmt == 'd' || *fmt == 'h'){
				int num = va_arg(args, int);
				to_num(num, dest, *fmt);
				while(*dest++);
				--dest;

			}else if(*fmt == 'c'){
				char ch = va_arg(args, char);
				*dest++ = ch;
			}
			++fmt;
		}
	}
}

