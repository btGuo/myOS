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

int     close(int);
int     chdir(const char *);
int     dup(int);
int     dup2(int, int);
int     execve(const char *, char *const [], char *const []);
pid_t   getpid(void);
off_t   lseek(int, off_t, int);
ssize_t write(int, const void *, size_t);
ssize_t read(int, void *, size_t);

#ifdef __cplusplus
}
#endif

#endif
