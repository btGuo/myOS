#include "syscall.h"
#include "print.h"
#include "thread.h"
#include "string.h"
#include "console.h"
#include "memory.h"
#include "fs_sys.h"
#include "process.h"
#include "sys_macro.h"

extern int32_t sys_execv(const char *path, const char **argv);

typedef void* syscall;
syscall syscall_table[syscall_nr];

uint32_t sys_getpid(void){
	return curr->pid;
}

#define __SYS(name) syscall_table[__NR_##name] = sys_##name

void sys_call_init(void){
	put_str("sys_call init\n");
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
	put_str("sys_call init done\n");
}

