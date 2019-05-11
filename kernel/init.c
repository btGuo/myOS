#include"init.h"
#include"print.h"
#include"interrupt.h"
#include"timer.h"
#include"memory.h"
#include"thread.h"
#include"console.h"

void init_all(){
	put_str("init_all start\n");
	idt_init();
	timer_init();
	mem_init();
	thread_init();
	console_init();
	put_str("init_all done\n");
}
