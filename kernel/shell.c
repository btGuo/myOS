#include <stdint.h>
#include <string.h>
#include <file.h>
#include <syscall.h>
#include <debug.h>

#define PROMPT(str) printf(prompt_fmt, str)
#define CMD_BUF_LEN 128
#define MAX_ARG_NR 64
#define MAX_PATH_LEN 128
const char *prompt_fmt = "[kyon@localhost %s]";
char cmd_buf[CMD_BUF_LEN];

/**
 * 读取一行
 * @param buf 输出缓冲区
 * @param size 最多读取字节数
 */
static void readline(char *buf, int32_t size){

	char *walk = buf;
	int32_t cnt = size;
	while(read(stdin_no, walk, 1) != -1 && cnt--){

		switch(*walk){
			case '\n':
			case '\r':
				*walk = '\0';
				putchar('\n');
				return;
			case '\b':
				if(buf[0] != '\b'){
					--walk;
					putchar('\b');
				}
				break;

			default:
				putchar(*walk);
				++walk;
		}
	}
	printf("can't read more chars, bufsize is %s\n", size);
}

/**
 * 解析字符串
 * @parma str 待解析的字符串
 * @param argv 解析结果，字符串数组
 * @param token 指定分隔符
 * @return 解析的字符串个数
 * 	@retval -1 失败
 *
 * @note 该函数会破坏原来的str
 */
static int32_t str_parse(char *str, char **argv, const char token){

	ASSERT(str != NULL);
	uint32_t argc = 0;
	while(argc < MAX_ARG_NR){
		argv[argc] = NULL;
		argc++;
	}
	argc = 0;
	char *walk = str;
	while(*walk){
		while(*walk == token)
			walk++;
		argv[argc] = walk;

		while(*walk != token && *walk)
			walk++;

		if(*walk){
			*walk = '\0'; 
			walk++;
		}
		if(argc >= MAX_ARG_NR){
			return -1;
		}
		argc++;
	}
	return argc;
}

/**
 * 将path转化为绝对路径，中间处理函数
 */
static void wash_path(const char *path, char *abs_path){

	char *arr[128];
	char *res[128];
	char **walk = res;
	uint32_t cnt = str_parse(path, arr, '/');
	uint32_t i = 0;
	for(; i < cnt; i++){
		if(strcmp(arr[i], ".") == 0){
			continue;
		}
		if(strcmp(arr[i], "..") == 0){
			walk--;
			if(walk < res)
				walk = res;
			continue;
		}
		*walk = arr[i];
		walk++;
	}
	memset(abs_path, 0, MAX_PATH_LEN);
	uint32_t len = res - walk;
	abs_path[0] = '/';
	i = 0;
	for(; i < len; i++){
		strcat(abs_path, res[i]);
		strcat(abs_path, "/");
	}
	//去掉最后的'/'
	abs_path[strlen(abs_path) - 1] = '\0';
}
			
/**
 * 将相对路径转换为绝对路径
 * @param path 相对路径
 * @param abs_path 处理结果，绝对路径
 */
void make_abs_path(const char *path, char *abs_path){
	char prefix[MAX_PATH_LEN];
	if(path[0] != '\0'){
		memset(prefix, 0, MAX_PATH_LEN);
		//获取当前路径
		/*
		if(getcwd(prefix, MAX_PATH_LEN) != NULL){
			//如果不是根目录
			if(!(prefix[0] == '/' && prefix[1] == '\0'))
				strcat(prefix, "/");
		}
		*/
	}
	//拼接字符串后可能空间不够
	strcat(prefix, path);
	
}

/**
 * shell
 */
void my_shell(){

	while(1){
		PROMPT("/");
		memset(cmd_buf, 0, CMD_BUF_LEN);
		readline(cmd_buf, CMD_BUF_LEN);
	}
	PANIC("shell : should not be here\n");
}

