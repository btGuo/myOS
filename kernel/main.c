#include"print.h"
#include"init.h"
#include"debug.h"
#include"memory.h"
#include"thread.h"

void k_thread_a(void *);
void k_thread_b(void *);
void k_thread_c(void *);
int main()
{
	put_str("hello kernel\n");
	init_all();
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
