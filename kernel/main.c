#include <init.h>
#include <kernelio.h>
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
#include <sys_macro.h>
#include <fs_sys.h>

extern void sys_call_init(void);
void init_all();

void test_kmalloc(){

	char *p[13];
	for(int i = 0; i < 13; i++)
		p[i] = kmalloc(i * 13 + 1);
	
	printk("alloc done\n");
	for(int i = 0; i < 13; i++){

		kfree(p[i]);
		printk("%d\t", i);
	}
	printk("free done\n");
}

void kernel_main(struct multiboot_info *info) {
	set_info(info);

	init_all();

	//test_kmalloc();

	//test_fmt();
	//test_memory();
	//test_hashtable();
	//test_thread();
	//test_fs();
	//test_exec();
	intr_enable();

	while(1);
}

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

//先放在这里了
int kernel_execve(const char *path, char *const argv[], char *const envp[]){

	int __res;
	__asm volatile("int $0x80"
			: "=a"(__res)
			: "0"(__NR_execve), "b"(path), "c"(argv), "d"(envp)
			: "memory");
	return (int)(__res);
}

void init(void){

	char *const argv[] = {"init", NULL};
	char *const envp[] = {"HOME=/home/root", NULL};

	//加载真正的init进程
	kernel_execve("/bin/init", argv, envp);

	//不应该跑到这里
	while(1);
}

