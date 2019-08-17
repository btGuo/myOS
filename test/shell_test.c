#include "string.h"
#include "stdint.h"

#define MAX_ARG_NR 10
#define MAX_PATH_LEN 128

/*
void test_str_parse() {

	char *arr[MAX_ARG_NR];
	char src[] = "cat test.cpp > tmp";
	int argc = str_parse(src, arr, ' ');
	printf("%d\n", argc);
	while(argc--){
		printf("arr : %s\n", arr[argc]);
	}
}

char paths[][MAX_PATH_LEN] = {  "/home/guobt/mine/OS", 
				"/usr/include/./..", 
				"/share/tmp/src/../test/uuu", 
				"/", 
				"/../../.", 
				"",
};

void test_wash_path() {
	
	int paths_size = 0;
	while(paths[paths_size][0])
		paths_size++;
	paths_size--;
	printf("test size : %d\n", paths_size);
	char output[MAX_PATH_LEN];
	int i = 0;
	for(; i < paths_size; i++){
		wash_path(paths[i], output);
		printf("abs_path : %s\n", output);
	}

}

*/
void test_shell(){
	//test_wash_path();
}

