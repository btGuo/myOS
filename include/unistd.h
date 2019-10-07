#ifndef __UNISTD_H
#define __UNISTD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

/** for lseek */

/** Set file offset to offset. */
#define SEEK_SET 1
/** Set file offset to current plus offset.*/
#define SEEK_CUR 2
/** Set file offset to EOF plus offset.*/
#define SEEK_END 3

/** file number of stderr,2. */
#define STDERR_FILENO 2
/** file number of stdin,0. */
#define STDIN_FILENO 0
/** file number of stdout,1. */
#define STDOUT_FILENO 1

/* for access  */

/* Test for existence of file. */
#define F_OK 8
/* Test for read permission.  */
#define R_OK 4
/* Test for write permission.  */
#define W_OK 2
/* Test for execute (search) permission.  */
#define X_OK 1


int          close(int);
int          chdir(const char *);
int          dup(int);
int          dup2(int, int);

int          execl(const char *, const char *, ...);
int          execle(const char *, const char *, ...);
int          execlp(const char *, const char *, ...);
int          execv(const char *, char *const []);
int          execvp(const char *, char *const []);
int          execve(const char *, char *const [], char *const []);

int 	     access(const char *path, int amode);
pid_t        fork(void);
pid_t        getpid(void);
char        *getcwd(char *, size_t);
off_t        lseek(int, off_t, int);
ssize_t      read(int, void *, size_t);
int          unlink(const char *);
ssize_t      write(int, const void *, size_t);
void        _exit(int);
int          pipe(int [2]);
uid_t        getuid(void);

//这个不是posix
void        *sbrk(size_t incr);


#ifdef __cplusplus
}
#endif

#endif
