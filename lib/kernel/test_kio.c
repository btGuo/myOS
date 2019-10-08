#ifdef __TEST
#include "kernelio.h"
#include "tty.h"
#include "stdint.h"

void test_atoi(){
	int32_t num = atoi("123");
	if(num != 123)
		terminal_writestr("test_atoi wrong\n");
	
	num = atoi("-425");
	if(num != -425)
		terminal_writestr("test_atoi wrong\n");
}

void test_itoa(){
	char str[32];
	int num = 4534;
	itoa(num, str, 'd');
	terminal_writestr(str);
}

void test_printf(){
	printk("%10s%10d%-10c\n", "width5", 2333, 'T');
	printk("super_block has already exist\n");
	printk("238sdlfj5728945uy298ykshfdkjalnsdflkjhq3wirhfwifndkszjdbf29yhrhsjakbndfjksandfjh2qiurhfi23nbrfcjwbdfklaewhrf");
}

void test_tty(){
	int cnt = 40;
	while(cnt--)
		printk("%d\n", 242);
}

void test_fmt(){
	test_printf();
}

#endif
