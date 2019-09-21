#include <stdint.h>
#include <sys_macro.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/utsname.h>

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
_syscall1(int32_t, close, int32_t, fd)
_syscall3(int32_t, read, int32_t, fd, void *, buf, uint32_t, count)
_syscall3(int32_t, write, int32_t, fd, char*, str, uint32_t, count)
_syscall3(int32_t, lseek, int32_t, fd, int32_t, offset, uint8_t, whence)
_syscall1(int32_t, unlink, const char *, path)
_syscall1(int32_t, rmdir, char *, path)
_syscall1(struct dirent *, readdir, struct Dir *, dir)
_syscall1(void , rewinddir, struct Dir *, dir)
_syscall1(int32_t, mkdir, char *, path)
_syscall1(int32_t, closedir, struct Dir *, dir)
_syscall1(struct   Dir *, opendir, char *, path)
_syscall2(int32_t, stat, const char *, path, struct stat *, st)
_syscall2(int32_t, execv, const char *, path, const char **, argv)
_syscall0(void, clear)
_syscall1(void, putchar, char, ch)
_syscall1(void, exit, int32_t, status)
_syscall1(int32_t, wait, int32_t, status)
_syscall2(char *, getcwd, char *, buf, uint32_t, size)
_syscall1(int32_t, dup, int32_t, fd)
_syscall2(int32_t, dup2, int32_t, oldfd, int32_t, newfd)
_syscall1(int, uname, struct utsname *, name)

/**
 * 这个有丶special
 */
int open(const char *path, int flag, ...){

	va_list args;
	va_start(args, flag);
	//拿出mode，可能不存在
	int mode = va_arg(args, int); 
	va_end(args);

	int __res;
	__asm volatile("int $0x80"
			: "=a"(__res)
			: "0"(__NR_open), "b"(path), "c"(flag), "d"(mode)
			: "memory");

	return __res;
}

