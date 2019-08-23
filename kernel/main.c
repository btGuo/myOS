#include "init.h"
#include "test.h"
#include "stdint.h"
#include "debug.h"
#include "syscall.h"
#include "interrupt.h"
#include "tty.h"
#include "stdio.h"

void init(void);

int main() {
	init_all();
	//test_fmt();
	//test_memory();
	//test_hashtable();
	///test_thread();
	//test_fs();
	//test_exec();
	//intr_enable();

	while(1);
	return 0;
}

void init(void){
	printf("init proc start\n");
	
	/*
	uint32_t pid = fork();
	if(pid){
		printf("I am father my pid is %d\n", getpid());
	}else {
		printf("I am child my pid is %d\n", getpid());
		int delay = 10000;
		while(delay--);
	}
	*/
	
	
	while(1);
}

