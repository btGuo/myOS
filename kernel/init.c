#include <init.h>
#include <debug.h>
#include <interrupt.h>
#include <timer.h>
#include <memory.h>
#include <thread.h>
#include <keyboard.h>
#include <tss.h>
#include <ide.h>
#include <fs.h>
#include <tty.h>

extern void sys_call_init(void);

void init_all(){
	terminal_init();
	printk("init_all start\n");

	tss_init();
	idt_init();
	mem_init();
	timer_init();

	keyboard_init();
	sys_call_init();
	
	ide_init();
	filesys_init();
	thread_init();
		
	printk("init_all done\n");
}
