#include "init.h"
#include "test.h"
#include "stdint.h"
#include "print.h"

void init(void);

int main() {
	init_all();

//	test_memory();
//	test_hashtable();
	test_thread();
	
	while(1);
	return 0;
}

void init(void){
	uint32_t pid = fork();
	if(pid){
		printf("I am father\n");
	}else {
		printf("I am child\n");
	}
	while(1);
}

