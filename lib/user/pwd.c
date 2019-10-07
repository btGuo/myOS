#include <pwd.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#define PWD_SZ 256 ///< 默认缓冲大小

#define __restore(buf)\
for(int i = 0; i < len; i++){ 	\
	if(buf[i] == '\0'){ 	\
		buf[i] = ':';   \
	} \
}

typedef int (*check_func)(const void *, char *, size_t);

int check_name(const void *arg, char *buf, size_t buflen){
	
	const char *name = (const char *)arg;
	size_t len = strlen(buf);

	char *savep; 
	char *token = strtok_r(buf, ":", &savep);
	if(token == NULL){
		return -1;
	}
	int ret = -1;
	if(strcmp(name, token) == 0){
		ret = 0;
	}

	//还原
	__restore(buf);
	return ret;
}

int check_uid(const void *arg, char *buf, size_t buflen){

	int uid = *((const int *)arg);
	size_t len = strlen(buf);

	char *savep;
	char *token = strtok_r(buf, ":", &savep);

	if(token == NULL){
		return -1;
	}
	int ret = -1;
	token = strtok_r(NULL, ":", &savep);
	token = strtok_r(NULL, ":", &savep);
	
	if(token == NULL){
		return -1;
	}
	if(uid == atoi(token)){
		ret = 0;
	}

	__restore(buf);
	return ret;
}

/**
 * getpwuid_r 和 getpwnam_r 主要调用函数
 * @param arg void* 类型，可能是name或者uid
 * @param pwd 输出结构体
 * @param buf pwd用的缓存区
 * @param buflen 缓存区长度
 * @param result 结果，如果成功，则保存的是pwd的指针，如果失败则为null
 * @param comp  比较函数用于比较name或者uid是否相等
 *
 * @attention 这里用的是strtok_r 分割字符串，如果passwd文件中有一项是空的
 * (即两个::)那么将会导致pwd里面的项错位
 */
int __getpwnX_r(const void *arg, struct passwd *pwd, char *buf, 
		size_t buflen, struct passwd **result, check_func comp){

	int fd = open("/etc/passwd", O_RDONLY);
	if(fd == -1){
		*result = NULL;
		return -1;
	}

	ssize_t rlen = 0;
	size_t rdl_len = PWD_SZ;
	char *rdl_buf = malloc(rdl_len);
	if(rdl_buf == NULL){

		*result = NULL;
		close(fd);
		return -1;
	}

	char *savep;
	const char *delim = ":";

	while((rlen = readline(&rdl_buf, &rdl_len, fd)) != 0){
 	
		if(comp(arg, rdl_buf, rdl_len) == 0){
		
			if(rlen > buflen){
				goto error;
			}

			memcpy(buf, rdl_buf, rlen);

			pwd->pw_name   = strtok_r(buf, delim, &savep);
			pwd->pw_passwd = strtok_r(NULL, delim, &savep);
			pwd->pw_uid    = atoi(strtok_r(NULL, delim, &savep));
			pwd->pw_gid    = atoi(strtok_r(NULL, delim, &savep));
			pwd->pw_info   = strtok_r(NULL, delim, &savep);
			pwd->pw_dir   = strtok_r(NULL, delim, &savep);
			pwd->pw_shell  = strtok_r(NULL, delim, &savep);

			*result = pwd;
			free(rdl_buf);
			close(fd);
			return 0;
		}
		memset(rdl_buf, 0, rdl_len);
	}

error:
	memset(buf, 0, buflen);
	*result = NULL;
	free(rdl_buf);
	close(fd);
	return -1;
}

int getpwnam_r(const char *name, struct passwd *pwd, char *buf, 
		size_t buflen, struct passwd **result){

	return __getpwnX_r(name, pwd, buf, buflen, result, check_name);
}


int getpwuid_r(uid_t uid, struct passwd *pwd, char *buf, 
		size_t buflen, struct passwd **result){

	return __getpwnX_r(&uid, pwd, buf, buflen, result, check_uid);
}

//这两个线程不安全，不实现
struct passwd *getpwuid(uid_t uid){
	//TODO
	return NULL;
}

struct passwd *getpwnam(const char *name){
	//TODO
	return NULL;
}






