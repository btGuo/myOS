#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int fd[2];
	if(pipe(fd) == -1){
		printf("pipe failed\n");
		while(1);
	}
	printf("fd1 %d\n", fd[0]);
	printf("fd2 %d\n", fd[1]);

	uint32_t pid = fork();
	if(pid){
		printf("I am father my pid is %d\n", getpid());

		close(fd[0]);
		size_t n = write(fd[1], "hello my child", 14);
		printf("write %d\n", n);
		close(fd[1]);
		
	}else {
		printf("I am child my pid is %d\n", getpid());

		close(fd[1]);
		char buf[32];
		size_t n = read(fd[0], buf, 32);
		if(n > 0){
			printf("read %d(bytes)\n", n);
			buf[n] = '\0';
			printf("from father:%s\n", buf);
		}
		close(fd[0]);
	}

	while(1);
}
