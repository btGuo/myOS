#include "stdint.h"
#include "sys_macro.h"
#include "dir.h"

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
_syscall0(uint32_t,getpid)
_syscall1(void*,   malloc, uint32_t, size)
_syscall1(void,    free, void *, ptr)
_syscall0(int32_t, fork)
_syscall2(int32_t, open, const char *, path, uint8_t, flag)
_syscall1(int32_t, close, int32_t, fd)
_syscall3(int32_t, read, int32_t, fd, void *, buf, uint32_t, count)
_syscall3(int32_t, write, int32_t, fd, char*, str, uint32_t, count)
_syscall3(int32_t, lseek, int32_t, fd, int32_t, offset, uint8_t whence)
_syscall1(int32_t, unlink, const char *, path)
_syscall1(int32_t, rmdir, char *, path)
_syscall2(int32_t, readdir, struct dir *, dir, struct dir_entry *, dir_e)
_syscall1(int32_t, rewinddir, struct dir *, dir)
_syscall1(int32_t, mkdir, char *path)
_syscall1(int32_t, closedir, struct dir *, dir)
_syscall1(struct dir *, opendir, char *, path)
_syscall2(int32_t, stat, const char *, path, struct stat *, st)
_syscall2(int32_t, execv, const char *, path, const char **, argv)



