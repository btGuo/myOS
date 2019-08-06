#include "thread.h"
#include "print.h"
#include "interrupt.h"


void k_thread_a(void *argv){
	while(1){
		printk("=====");
	}
}

void k_thread_b(void *argv){
	while(1){
		printk("#####");
	}
}

void k_thread_c(void *argv){
	while(1){
		printk("/////");
	}
}

void u_prog_a(void *argv){
	while(1){
		printf("####");
	}
}

void u_prog_b(void *argv){
	while(1){
		printf("=====");
	}
}

void test_thread(){

	process_execute(u_prog_a, "user_prog_a");
	process_execute(u_prog_b, "user_prog_b");
	thread_start("k_thread_a", 31, k_thread_a, NULL);
	thread_start("k_thread_b", 31, k_thread_b, NULL);
	thread_start("k_thread_c", 31, k_thread_c, NULL);
	
	intr_enable();
}
