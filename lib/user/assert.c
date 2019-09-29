#include <stdio.h>

void __assert_handler(char *filename, int line, const char *func, const char *condition){

	//TODO send signal to kernel
	
	printf("\n========= error ============\n");
	printf("filename : %s\n", filename);
	printf("line : %d\n", line);
	printf("function : %s\n", func);
	printf("condition : %s\n", condition);

	while(1);
}
