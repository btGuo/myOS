#include "stdint.h"
#include "sys_macro.h"

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


//不需要分号
_syscall0(uint32_t, getpid)
_syscall3(uint32_t, write, int32_t, fd, char*, str, uint32_t, count)
_syscall1(void*, malloc, uint32_t, size)
_syscall1(void,  free, void *, ptr)
_syscall0(int32_t, fork)

