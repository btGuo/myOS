#include "stdio.h"
#include "syscall.h"

int main(){
	int pid = fork();
	if(pid){
		printf("I am father my pid is %d\n", getpid());
		wait(0);
		while(1);
	}else {
		printf("I am child my pid is %d\n", getpid());
		exit(0);
	}
	return 0;
}

