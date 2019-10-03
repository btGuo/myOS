#ifndef __UNISTD_H
#define __UNISTD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/** Set file offset to offset. */
#define SEEK_SET 1
/** Set file offset to current plus offset.*/
#define SEEK_CUR 2
/** Set file offset to EOF plus offset.*/
#define SEEK_END 3

int          close(int);
int          chdir(const char *);
int          dup(int);
int          dup2(int, int);
int          execve(const char *, char *const [], char *const []);
pid_t        fork(void);
pid_t        getpid(void);
char        *getcwd(char *, size_t);
off_t        lseek(int, off_t, int);
ssize_t      read(int, void *, size_t);
int          unlink(const char *);
ssize_t      write(int, const void *, size_t);
void        _exit(int);

//这个不是posix
void        *sbrk(size_t incr);


#ifdef __cplusplus
}
#endif

#endif
