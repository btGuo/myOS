#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "print.h"
#include "list.h"
#include "process.h"
#include "interrupt.h"
#include "stdint.h"

void u_prog_a(void);
void u_prog_b(void);

int main()
{

	init_all();
	process_execute(u_prog_a, "user_prog_a");
	process_execute(u_prog_b, "user_prog_b");
	
	intr_enable();
	while(1);
	return 0;
}

void u_prog_a(void){
	while(1);
}

void u_prog_b(void){
	while(1);
}
