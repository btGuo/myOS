#include <stdlib.h>
#include <unistd.h>
#include "memlib.h"

static char *mem_heap = NULL;
static char *mem_brk = NULL;
static char *mem_max_addr = NULL;

#define MAX_HEAP (1 << 22) ///< 4M

void mem_init(void){
	mem_heap = (char *)sbrk(MAX_HEAP);
	mem_brk = mem_heap;
	mem_max_addr = mem_heap + MAX_HEAP;
}

void *mem_sbrk(int incr){
	
	if(mem_heap == NULL){
		mem_init();
	}

	char *old_brk = mem_brk;

	if((incr < 0 || mem_brk + incr > mem_max_addr)){
		return NULL;
	}

	mem_brk += incr;
	return old_brk;
}

