#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/wait.h>

int main(int argc, char *argv[], char *envp[])
{
	open("/dev/tty0", O_RDWR);
	dup(0);
	dup(0);
	
	uint32_t pid = fork();
	if(pid){
		printf("I am father my pid is %d\n", getpid());
		wait(NULL);
		
	}else {
		printf("I am child my pid is %d\n", getpid());

		char *argv[] = {"shell", NULL};
		if(execvp("shell", argv) == -1){
			printf("execvp wrong\n");
		}
		/** 不会到这里 */
	}
	
	while(1);
}
