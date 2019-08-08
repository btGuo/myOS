#include "syscall.h"
#include "stdio.h"
int main(){
	char buf[10];
	vsprintf(buf, "%s world\n", "hello");
	while(1);
	return 0;
}

