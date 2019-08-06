#include "init.h"
#include "test.h"

int main() {
	init_all();

//	test_memory();
//	test_hashtable();
	test_thread();
	
	while(1);
	return 0;
}


