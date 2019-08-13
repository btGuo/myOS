#include "stdint.h"

const char *prompt = "[kyon@localhost %s]";
#define PROMPT(str) printf(prompt, str)

static void readline(char *buf, int32_t size){

	char *walk = buf;
	int32_t cnt = size;
	while(read(stdin_no, walk, 1) != -1 && cnt--){

		switch(*walk){
			case '\n':
			case '\r':
				*walk = '\0';
				putchar('\n');
				return;
			case '\b':
				if(buf[0] != '\b'){
					--walk;
					putchar('\b');
				}
				break;

			default:
				putchar(*walk);
				++walk;
		}
	}
	printf("can't read more chars, bufsize is %s\n", size);
}






