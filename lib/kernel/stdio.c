#include "stdarg.h"
#include "string.h"
#include "stdint.h"
#include "global.h"
#include "syscall.h"

static const char map[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8',\
	'9', 'a', 'b', 'c', 'd', 'e', 'f'};

#define isdigit(x) ((x) >= '0' && (x) <= '9')
#define MAX_ALIGN 8

/**
 * 字符串转数字
 */
int32_t atoi(char *src){
	uint32_t len = strlen(src);
	bool neg = false;
	if(*src == '-'){
		neg = true;
		++src;
	}
	if(*src == '+'){
		++src;
	}
	int32_t ans = 0;
	uint32_t i = 0;
	for(; i < len; i++){
		ans *= 10;
		ans += src[i] - '0';
	}
	if(neg) ans = -ans;
	return ans;
}

/**
 * 数字转字符串, 可选十进制和十六进制
 */
void itoa(int num, char *dest, char mode){
	char _tmp[1024];
	//这里多加一, 后面while舒服点
	char *tmp = _tmp + 1;
	if(mode == 'd'){
		if(num < 0){
			num = -num;
			*dest++ = '-';
		}
		if(num == 0){
			*dest++ = '0';
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

	}else if(mode == 'x'){
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

/**
 * 字符串对齐
 * @param str 目标字符串
 * @param dest 处理结果
 * @param width 宽度
 * @param left 是否左对齐
 */
void str_align(char *str, char *dest, uint32_t width, bool left){
	uint32_t len = strlen(str);
	if(len >= width){
		strcpy(dest, str);
		return;
	}

	if(left){
		width -= len;
		while(len--)
			*dest++ = *str++;
		while(width--)
			*dest++ = ' ';
	}else {
		width -= len;
		while(width--)
			*dest++ = ' ';
		while(len--)
			*dest++ = *str++;
	}
	*dest = '\0';
}

/**
 * 简单到不能再简单的格式化字符串 @_@
 * @note 支持%s, %d, %x, %c. 宽度对齐，左右对齐
 */
void vsprintf(char *buf, const char *fmt, va_list args){
	char *dest = buf;
	char width[MAX_ALIGN];
	bool left = false;
	//!!!注意这里的大小可能会溢出
	char content[128];
	while(*fmt){
		if(*fmt != '%'){
			*dest++ = *fmt++;
			*dest = '\0';
		}else{
			++fmt;

			//对齐方式
			if(*fmt == '-'){
				left = true;
				++fmt;
			}
			else left = false;

			// 提取对齐宽度
			char *_width = width;
			while(isdigit(*fmt))
				*_width++ = *fmt++;
			*_width = '\0';

			
			//提取到content中
			if(*fmt == 's'){
				char *str = va_arg(args, char*);	
				char *cont = content;
				while((*cont++ = *str++));
				
			}else if(*fmt == 'd' || *fmt == 'x'){
				int num = va_arg(args, int);
				itoa(num, content, *fmt);

			}else if(*fmt == 'c'){
				char ch = va_arg(args, char);
				content[0] = ch;
				content[1] = '\0';
			}

			str_align(content, dest, atoi(width), left);
			while(*dest)
				dest++;
			++fmt;
		}
	}
}

/**
 * 格式化字符串
 */
void sprintf(char *buf, const char *fmt, ...){
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
}

/**
 * 格式化输出
 */
uint32_t printf(const char *fmt, ...){
	va_list args;
	
	char buf[1024] = {0};
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	//和printk区别在于系统调用
	return write(1, buf, strlen(buf));
}
