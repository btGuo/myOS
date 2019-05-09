#include"print.h"
#include"init.h"
#include"debug.h"
#include"memory.h"
#include"thread.h"

void k_thread_a(void *);
void k_thread_b(void *);
void k_thread_c(void *);
void k_thread_d(void *);
int main()
{
	put_str("hello kernel\n");
	init_all();
	thread_start("k_thread_a", 4, k_thread_a, "/");
	thread_start("k_thread_b", 4, k_thread_b, "*");
	thread_start("k_thread_c", 4, k_thread_c, "@");
	thread_start("k_thread_d", 4, k_thread_d, "-");
	intr_enable();
	while(1);
	return 0;
}

void k_thread_a(void *arg){
	char *para = arg;
	while(1){
		put_str(para);
	}
}
void k_thread_b(void *arg){
	char *para = arg;
	while(1){
		put_str(para);
	}
}
void k_thread_c(void *arg){
	char *para = arg;
	while(1){
		put_str(para);
	}
}
void k_thread_d(void *arg){
	char *para = arg;
	while(1){
		put_str(para);
	}
}
