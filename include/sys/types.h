#ifndef __SYS_TYPES_H
#define __SYS_TYPES_H

/** posix */

#include <stdint.h>

// Used for file block counts.
typedef uint32_t blkcnt_t;

// Used for block sizes.
typedef uint32_t blksize_t;

// [XSI] [Option Start] Used for system times in clock ticks or CLOCKS_PER_SEC; see <time.h>. [Option End]
typedef uint32_t clock_t;

// [TMR] [Option Start] Used for clock ID type in the clock and timer functions. [Option End]
typedef uint32_t clockid_t;

// Used for device IDs.
typedef uint32_t dev_t;

// [XSI] [Option Start] Used for file system block counts. [Option End]
typedef uint32_t fsblkcnt_t;

// [XSI] [Option Start] Used for file system file counts. [Option End]
typedef uint32_t fsfilcnt_t;

// Used for group IDs.
typedef uint32_t gid_t;

// [XSI] [Option Start] Used as a general identifier; can be used to contain at least a pid_t, uid_t, or gid_t. [Option End]
typedef uint32_t id_t;

// Used for file serial numbers.
typedef uint32_t ino_t;

// [XSI] [Option Start] Used for XSI interprocess communication. [Option End]
typedef uint32_t key_t;

// Used for some file attributes.
typedef uint32_t mode_t;

// Used for link counts.
typedef uint32_t nlink_t;

// Used for file sizes.
typedef uint32_t off_t;

// Used for process IDs and process group IDs.
typedef int32_t pid_t;

// Used for time in seconds.
typedef uint32_t time_t;

// Used for user IDs.
typedef int32_t uid_t;
//Used for sizes of objects.
//
typedef uint32_t size_t;

//Used for a count of bytes or an error indication.
typedef int32_t ssize_t;
#endif
