#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "print.h"
#include "list.h"
#include "process.h"
#include "interrupt.h"
#include "stdint.h"
#include "syscall.h"
#include "fs.h"


void u_prog_a(void);
void u_prog_b(void);
void k_thread_a(void);
void k_thread_b(void);
void k_thread_c(void);

int main() {
	init_all();
//	process_execute(u_prog_a, "user_prog_a");
//	process_execute(u_prog_b, "user_prog_b");
//	thread_start("k_thread_a", 31, k_thread_a, NULL);
//	thread_start("k_thread_b", 31, k_thread_b, NULL);
//	thread_start("k_thread_c", 31, k_thread_c, NULL);
	
//	intr_enable();


	uint32_t fd = sys_open("/ftest", O_RDWR);
	printk("fd %d\n", fd);
	char buf[100];
	memset(buf, 0, 100);
//	sys_write(fd, "hello world\n", 13);
//	sys_read(fd, buf, 18);
	//printk("content %s\n", buf);
	sys_close(fd);
	sync();

	while(1);
	return 0;
}

void u_prog_a(void){
	while(1){
		printf("####");
	}
}

void u_prog_b(void){
	while(1){
		printf("=====");
	}
}

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
