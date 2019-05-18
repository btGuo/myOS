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

void k_thread_a(void *);
void k_thread_b(void *);
void u_prog_a(void);
void u_prog_b(void);

int a = 0, b = 0;

int main()
{

	init_all();
	process_execute(u_prog_a, "user_prog_a");
	printk("%s\n", "done prepare");
	
	intr_enable();
	while(1);
	return 0;
}

void k_thread_a(void *arg){
	char *para = arg;
	while(1){
		printk("%s", para);
	}
}
void k_thread_b(void *arg){
	char *para = arg;
	while(1){
		printk("%s", para);
	}
}
void u_prog_a(void){
	while(1){
		printk("%s", "aaa");
	}
}

void u_prog_b(void){
	while(1){
		++b;
	}
}
