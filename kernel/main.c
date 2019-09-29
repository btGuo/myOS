#include <init.h>
#include <debug.h>
#include <interrupt.h>
#include <timer.h>
#include <memory.h>
#include <thread.h>
#include <tss.h>
#include <ide.h>
#include <fs.h>
#include <tty.h>
#include <char_dev.h>
#include <test.h>
#include <stdint.h>
#include <multiboot.h>

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

void kernel_main(struct multiboot_info *info) {
	set_info(info);
	init_all();

//	PANIC("kernel main\n");
	//test_fmt();
	//test_memory();
	//test_hashtable();
	//test_thread();
	//test_fs();
	//test_exec();
	intr_enable();

	while(1);
	//return 0;
}

void init(void){

	while(1);
}

