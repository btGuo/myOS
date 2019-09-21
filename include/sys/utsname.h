#ifndef __SYS_UTSNAME_H
#define __SYS_UTSNAME_H

#define UTSBUF 64

struct utsname
{
	/** Name of this implementation of the operating system.*/
	char sysname[UTSBUF];

	/** Name of this node within the communications 
                 network to which this node is attached, if any. */
	char nodename[UTSBUF];

	/** Current release level of this implementation.*/
	char release[UTSBUF];

	/** Current version level of this release.*/
	char version[UTSBUF];

	/** Name of the hardware type on which the system is running.*/
	char machine[UTSBUF];
};

extern int uname(struct utsname *);

#endif
