#include "init.h"
#include "test.h"
#include "stdint.h"
#include "print.h"
#include "syscall.h"
#include "interrupt.h"

void init(void);

int main() {
	init_all();

	//test_memory();
	//test_hashtable();
	///test_thread();
	//test_fs();
	test_exec();
	
	intr_enable();
	while(1);
	return 0;
}

void init(void){
	printf("init proc start\n");
	/*
	uint32_t pid = fork();
	if(pid){
		printf("I am father\n");
	}else {
		printf("I am child\n");
	}
	*/
	while(1);
}

