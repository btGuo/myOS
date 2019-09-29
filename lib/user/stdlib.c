#include <mm.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

static int mm_mark = 0; //这个应该是原子的

static inline void check_mark(){
	if(!mm_mark){
		mm_init();
		mm_mark = 1;
	}
}

int atoi(const char *src){

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

void *malloc(size_t size){
	check_mark();
	return mm_alloc(size);
}

void free(void *p){

	mm_free(p);
}

void *realloc(void *p, size_t size){

	check_mark();
	return mm_realloc(p, size);
}

void *calloc(size_t lens, size_t size){

	check_mark();
	return mm_calloc(lens, size);
}

#define RDLINE_BUFSZ 1024

static size_t rdline_bsz = 64; ///< readline 默认每次读取大小

void rdline_set_bsz(size_t bsz){

	rdline_bsz = bsz;
}

/**
 * 从指定文件中读取一行
 * @param fd 文件描述符
 * @param buf 缓冲区二级指针
 * @param buflen 缓冲区大小
 * @return 读取字节数
 *
 * @note 如果*buf为空，则会申请新空间用，使用完后需要用户自己释放
 * 该空间，同时buflen会被设置了新buf的长度，如果旧缓冲区长度不够
 * 则会由realloc重新分配一块大的空间，只到读取一行结束或者文件结束
 *
 * @attention 这里用了一些特殊的操作，每次调用read都读取一定的字节数，
 * 而不是每次读取一个字节。
 */
ssize_t readline(int fd, char **buf, size_t *buflen){

	if(*buf == NULL || *buflen == 0){

		*buf = malloc(RDLINE_BUFSZ);
		*buflen = RDLINE_BUFSZ;
		
		//分配失败
		if(*buf == NULL){

			return -1;
		}
	}

	size_t batchsz = rdline_bsz;
	batchsz = batchsz < *buflen ? batchsz :*buflen;
	ssize_t total = 0;
	ssize_t rlen = 0;

	char *bufp = *buf;
	while((rlen = read(fd, bufp, batchsz))){

		if(rlen < 0){
			return -1;
		}

		for(int i = 0; i < rlen; i++){

			if(bufp[i] == '\n'){

				bufp[i] = '\0';
				lseek(fd, -(rlen - i - 1) ,SEEK_CUR);
				total += i;
				return total;
			}
		}

		total += rlen;
		bufp += rlen;

		if(total + batchsz > *buflen){

			//重新分配内存，翻倍
			*buf = realloc(*buf, *buflen * 2);
			if(*buf == NULL){
				return -1;
			}
			
			*buflen = *buflen * 2;
			bufp = *buf + total;
		}

	}

	return total;
}
