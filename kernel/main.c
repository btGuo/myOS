#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "print.h"
#include "list.h"

void k_thread_a(void *);
void k_thread_b(void *);
void k_thread_c(void *);
void k_thread_d(void *);
int main()
{
	init_all();
	thread_start("k_thread_a", 6, k_thread_a, "thA_");
	thread_start("k_thread_b", 6, k_thread_b, "thB_");
	//thread_start("k_thread_c", 6, k_thread_c, "thC_");
	//thread_start("k_thread_d", 6, k_thread_d, "thD_");
	intr_enable();
	while(1);
	
	return 0;
}

void k_thread_a(void *arg){
	char *para = arg;
	while(1){
		printk("%s", para);
		//console_write(para);
	}
}
void k_thread_b(void *arg){
	char *para = arg;
	while(1){
		printk("%s", para);
		//console_write(para);
	}
}
void k_thread_c(void *arg){
	char *para = arg;
	while(1){
		printk("%s", para);
		//console_write(para);
	}
}
void k_thread_d(void *arg){
	char *para = arg;
	while(1){
		printk("%s", para);
		//console_write(para);
	}
}
