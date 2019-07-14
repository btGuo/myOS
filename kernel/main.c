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
#include "fs_sys.h"


void u_prog_a(void);
void u_prog_b(void);
void k_thread_a(void);
void k_thread_b(void);
void k_thread_c(void);
//filename qqqq awqq bwqq

int main() {
	init_all();
//	process_execute(u_prog_a, "user_prog_a");
//	process_execute(u_prog_b, "user_prog_b");
//	thread_start("k_thread_a", 31, k_thread_a, NULL);
//	thread_start("k_thread_b", 31, k_thread_b, NULL);
//	thread_start("k_thread_c", 31, k_thread_c, NULL);
	
//	intr_enable();

/*	
	printk("here\n");
	int fd = sys_open("/dwqq", O_CREAT | O_RDWR);

	char buf[1024];
	int i = 1024;
	while(i--)
		buf[i] = 'i';
	i = 1;
	printk("here\n");
	while(i--)
		sys_write(fd, buf, 1024);

	printk("here\n");
	sys_close(fd);
	
	printk("here\n");

	if(sys_unlink("/dwqq") == -1){
		printk("error");
	}
	printk("here\n");
*/

	// /dir1 /dir1/a  /dir1/b  /dir1/fa 

//	int fd = sys_open("/dir1/fa", O_CREAT | O_RDWR);
//	sys_close(fd);

	struct dir *dir = sys_opendir("/dir1");
	if(!dir){
		printk("open failed\n");
	}
	struct dir_entry dir_e;
	while(sys_readdir(dir, &dir_e) != -1){
		printk("dir_e.filename %s    ", dir_e.filename);
	}
	printk("\n");
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
