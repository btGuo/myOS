#include <init.h>
#include <test.h>
#include <stdint.h>
#include <debug.h>
#include <syscall.h>
#include <interrupt.h>
#include <tty.h>
#include <stdio.h>
#include <clock.h>
#include <multiboot.h>
#include <fs.h>
#include <fcntl.h>

void init(void);

void kernel_main(struct multiboot_info *info) {
	set_info(info);
	init_all();

//	PANIC("kernel main\n");
	//test_fmt();
	//test_memory();
	//test_hashtable();
	//test_thread();
	//test_fs();
	//test_exec();
	intr_enable();

	while(1);
	//return 0;
}

void init(void){
	printf("init proc start\n");
	
	open("/dev/tty0", O_RDWR);
	dup(0);
	dup(0);
	
	uint32_t pid = fork();
	if(pid){

		printf("I am father my pid is %d\n", getpid());
		char *file = "/home/test.txt";
		int fd = open(file, O_CREAT | O_RDONLY, S_IRUSR);
		if(fd == -1){

			printf("open file %s failed\n", file);
		}else {
			
			close(fd);
		}

		if((fd = open(file, O_RDWR)) == -1){

			printf("open file %s failed\n", file);
		}

		
	}else {
		printf("I am child my pid is %d\n", getpid());
		int delay = 10000;
		while(delay--);
	}
	
	while(1);
}

void login(){

	printf("username:");
}

