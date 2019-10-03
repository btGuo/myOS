#ifndef __SYS_MACROS_H
#define __SYS_MACROS_H

#define __NR_getpid    1
//#define __NR_malloc    2
//#define __NR_free      3
#define __NR_fork      4
#define __NR_open      5 
#define __NR_close     6
#define __NR_read      7
#define __NR_write     8
#define __NR_lseek     9
#define __NR_unlink    10
#define __NR_rmdir     11 
#define __NR_readdir   12
#define __NR_rewinddir 13
#define __NR_mkdir     14
#define __NR_closedir  15
#define __NR_opendir   16
#define __NR_execve    17
#define __NR_stat      18
#define __NR_clear     19
#define __NR_putchar   20
#define __NR_exit      21
#define __NR_wait      22
#define __NR_getcwd    23
#define __NR_dup       24
#define __NR_dup2      25
#define __NR_uname     26
#define __NR_sbrk      27
#define __NR_chdir     28
#define __NR__exit     29
#define __NR_chmod     30

#define syscall_nr 72

#endif
