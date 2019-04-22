#include"print.h"
#include"init.h"
int main()
{
	put_str("hello kernel\n");
	init_all();
	asm volatile("sti");	
	while(1);
	return 0;
}
