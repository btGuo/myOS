#ifndef __POSIX_WAIT_H
#define __POSIX_WAIT_H

#ifdef __cplusplus
extern "C"{
#endif

#include <sys/types.h>
pid_t  wait(int *);
pid_t  waitpid(pid_t, int *, int);

#ifdef __cplusplus
}
#endif

#endif
