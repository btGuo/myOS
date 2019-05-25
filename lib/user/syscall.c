#include "syscall.h"
#include "print.h"
#include "thread.h"
#include "string.h"
#include "console.h"

#define __NR_getpid 0
#define __NR_malloc 1
#define __NR_write 2

#define _syscall0(type, name)\
type name(void)\
{\
	int __res;\
	__asm volatile("int $0x80"\
					: "=a"(__res) \
					: "0"(__NR_##name));\
	return (type)(__res);\
}

#define _syscall1(type, name, type1, arg1)\
type name(type1 arg1)\
{\
	int __res;\
	__asm volatile("int $0x80"\
					: "=a"(__res)\
					: "0"(__NR_##name), "b"(arg1)\
					: "memory");\
	return (type)(__res);\
}

#define _syscall2(type, name, type1, arg1, type2, arg2)\
type name(type1 arg1, type2 arg2)\
{\
	int __res;\
	__asm volatile("int $0x80"\
					: "=a"(__res)\
					: "0"(__NR_##name), "b"(arg1), "c"(arg2)\
					: "memory");\
	return (type)(__res);\
}

#define _syscall3(type, name, type1, arg1, type2, arg2, type3, arg3)\
type name(type1 arg1, type2 arg2, type3 arg3)\
{\
	int __res;\
	__asm volatile("int $0x80"\
					: "=a"(__res)\
					: "0"(__NR_##name), "b"(arg1), "c"(arg2), "d"(arg3)\
					: "memory");\
	return (type)(__res);\
}

#define syscall_nr 72
typedef void* syscall;
syscall syscall_table[syscall_nr];
extern struct task_struct *curr;

//不需要分号
_syscall0(uint32_t, getpid)
_syscall1(uint32_t, write, char*, str)
_syscall1(void*, malloc, uint32_t, size)

uint32_t sys_getpid(void){
	return curr->pid;
}

uint32_t sys_write(char *str){
	console_write(str);
	return strlen(str);
}

void sys_call_init(void){
	put_str("sys_call init\n");
	syscall_table[__NR_getpid] = sys_getpid;
	syscall_table[__NR_write] = sys_write;
	syscall_table[__NR_malloc] = sys_malloc;
	put_str("sys_call init done\n");
}

