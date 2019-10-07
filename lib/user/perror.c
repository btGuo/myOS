#include <stdio.h>
#include <errno.h>
#include <assert.h>

const char *error_mess[] = 
{

	"Argument list too long",

	"Permission denied",

	"Address in use",

	"Address not available",

	"Address family not supported",

	"Resource unavailable, try again (may be the same value as as EWOULDBLOCK)",

	"Connection already in progress",

	"Bad file descriptor",

	"Bad message",

	"Device or resource busy",

	"Operation canceled",

	"No child processes",

	"Connection aborted",

	"Connection refused",

	"Connection reset",

	"Resource deadlock would occur",

	"Destination address required",

	"Mathematics argument out of domain of function",

	"Reserved",

	"File exists",

	"Bad address",

	"File too large",

	"Host is unreachable",

	"Identifier removed",

	"Illegal byte sequence",

	"Operation in progress",

	"Interrupted function",

	"Invalid argument",

	"I/O error",

	"Socket is connected",

	"Is a directory",

	"Too many levels of symbolic links",

	"Too many open files",

	"Too many links",

	"Message too large",

	"Reserved",

	"Filename too long",

	"Network is down",

	"Connection aborted by network",

	"Network unreachable",

	"Too many files open in system",

	"No buffer space available",

	"[XSR]  Option Start No message is available on the STREAM head read queue" 

	"No such device",

	"No such file or directory",

	"Executable file format error",

	"No locks available",

	"Reserved",

	"Not enough space",

	"No message of the desired type",

	"Protocol not available",

	"No space left on device",

	"[XSR]  Option Start No STREAM resources" 

	"[XSR]  Option Start Not a STREAM" 

	"Function not supported",

	"The socket is not connected",

	"Not a directory",

	"Directory not empty",

	"Not a socket",

	"Not supported",

	"Inappropriate I/O control operation",

	"No such device or address",

	"Operation not supported on socket",

	"Value too large to be stored in data type",

	"Operation not permitted",

	"Broken pipe",

	"Protocol error",

	"Protocol not supported",

	"Protocol wrong type for socket",

	"Result too large",

	"Read-only file system",

	"Invalid seek",

	"No such process",

	"Reserved",

	"[XSR] Option Start Stream ioctl() timeout" 

	"Connection timed out",

	"Text file busy",

	"Operation would block (may be the same value as  [EAGAIN])",

	"Cross-device link",
};

static const size_t mess_len = sizeof(error_mess) / sizeof(char *);

void perror(const char *str)
{
	assert(errno >= 0 && errno < mess_len);  
	printf("%s:%s", str, error_mess[errno]);
}
