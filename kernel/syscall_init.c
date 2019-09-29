#include <syscall.h>
#include <debug.h>
#include <thread.h>
#include <string.h>
#include <memory.h>
#include <fs_sys.h>
#include <process.h>
#include <sys_macro.h>
#include <tty.h>
#include <sys/utsname.h>

#define OSNAME "myOS"
#define NODENAME "k-1"
#define RELEASE "0.10"
#define VERSION "0.10"
#define MACHINE "i386"

extern void sys_exit(int32_t status);
extern pid_t sys_wait(int32_t status);
extern int32_t sys_execv(const char *path, const char **argv);
extern int32_t sys_dup2(int32_t oldfd, int32_t newfd);
extern int32_t sys_dup(int32_t fd);
extern void *sys_sbrk(uint32_t incr);

typedef void* syscall;
syscall syscall_table[syscall_nr];

uint32_t sys_getpid(void){
	return curr->pid;
}

int sys_uname(struct utsname *name){

	strcpy(name->sysname,  OSNAME);
	strcpy(name->nodename, NODENAME);
	strcpy(name->release,  RELEASE);
	strcpy(name->version,  VERSION);
	strcpy(name->machine,  MACHINE);
}

void sys_clear(){
	terminal_clear();
}

void sys_putchar(char ch){
	terminal_putchar(ch);
}

#define __SYS(name) syscall_table[__NR_##name] = sys_##name

void sys_call_init(void){
	printk("sys_call init\n");
	__SYS(getpid);
	__SYS(malloc);
	__SYS(free);
	__SYS(fork);
	__SYS(open);
	__SYS(close);
	__SYS(read);
	__SYS(write);
	__SYS(lseek);
	__SYS(unlink);
	__SYS(rmdir);
	__SYS(readdir);
	__SYS(rewinddir);
	__SYS(mkdir);
	__SYS(closedir);
	__SYS(opendir);
	__SYS(execv);
	__SYS(stat);
	__SYS(clear);
	__SYS(putchar);
	__SYS(wait);
	__SYS(exit);
	__SYS(getcwd);
	__SYS(dup);
	__SYS(dup2);
	__SYS(uname);
	__SYS(sbrk);
	printk("sys_call init done\n");
}

