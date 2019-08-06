#include "init.h"
#include "print.h"
#include "interrupt.h"
#include "timer.h"
#include "memory.h"
#include "thread.h"
#include "console.h"
#include "keyboard.h"
#include "tss.h"
#include "syscall.h"
#include "ide.h"
#include "fs.h"

void init_all(){
	put_str("init_all start\n");
	idt_init();
	timer_init();
	console_init();
	mem_init();
/*
	thread_init();
	keyboard_init();
	tss_init();
	sys_call_init();
	ide_init();
	filesys_init();
*/
	put_str("init_all done\n");
}
