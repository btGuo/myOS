#include "thread.h"
#include "print.h"
#include "interrupt.h"
#include "string.h"
#include "process.h"

void k_thread_a(void){
	while(1){
		printk("=====");
	}
}

void k_thread_b(void){
	while(1){
		printk("#####");
	}
}

void k_thread_c(void){
	while(1){
		printk("/////");
	}
}

void u_prog_a(void){
	printf("u_prog_a\n");
	uint32_t size = 1024 * 8;
	char data[size];
	uint32_t i = 0;
	for(; i < size; ++i){
		data[i] = 'A';
	}
	data[100] = '\0';
	printf("%s", data);

	while(1);
}

void u_prog_b(void){
	char *data = "u_prog_b";
	printf("%s\n", data);
	while(1);
}

void test_thread(){

	process_execute(u_prog_a, "user_prog_a");
	//process_execute(u_prog_b, "user_prog_b");
//	thread_start("k_thread_a", 31, k_thread_a, NULL);
//	thread_start("k_thread_b", 31, k_thread_b, NULL);
//	thread_start("k_thread_c", 31, k_thread_c, NULL);
	
	intr_enable();
}
