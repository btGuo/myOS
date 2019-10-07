#include <stdint.h>
#include <sys_macro.h>
#include <dirent.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <sys/utsname.h>
#include <sys/types.h>

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

//unistd.h
_syscall0(pid_t, getpid)
_syscall0(pid_t, fork)
_syscall1(int,     close, int, fd)
_syscall3(ssize_t, read,  int, fd, void *,       buf, size_t, count)
_syscall3(ssize_t, write, int, fd, const void *, str, size_t, count)
_syscall3(off_t,   lseek, int, fd, off_t,        off, int,    whence)
_syscall1(int, unlink, const char *, path)
_syscall1(int, dup,  int, fd)
_syscall2(int, dup2, int, oldfd, int, newfd)
_syscall3(int, execve, const char *, path, char *const *, argv, char *const *, envp)
_syscall1(int, chdir,  const char *, path)
_syscall2(char *, getcwd, char *, buf, size_t, size)
_syscall1(int, rmdir, char *, path)
_syscall1(void, _exit, int, status)
_syscall1(void *, sbrk, size_t, incr)
_syscall1(int, pipe, int *, fd)
_syscall0(uid_t, getuid);

//dirent.h
_syscall1(struct dirent *, readdir, DIR *, dir)
_syscall1(void , rewinddir, DIR *, dir)
_syscall1(int, closedir, DIR *, dir)
_syscall1(DIR *, opendir, const char *, path)

//sys/stat.h
_syscall2(int, stat,  const char *, path, struct stat *, st)
_syscall2(int, mkdir, const char *, path, mode_t, mode)
_syscall2(int, chmod, const char *, path, mode_t, mode)

//sys/wait.h
_syscall1(pid_t, wait, int *, status)

//stdio.h
_syscall1(int, putchar, int, ch)

//sys/utsname.h
_syscall1(int, uname, struct utsname *, name)

//stdlib.h
_syscall1(void, exit, int, status)

//fcntl.h，这个单独写出来，得解包参数
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
