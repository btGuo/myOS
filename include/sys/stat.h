#ifndef __STAT_H
#define __STAT_H

#include <sys/types.h>
/** posix */

#define FILETYPE_SHIFT 12
// 这里和linux保持一致

// Type of file. 
#define S_IFMT (15 << FILETYPE_SHIFT) 

//Block special.
#define S_IFBLK  (6 << FILETYPE_SHIFT)

//Character special.
#define S_IFCHR  (2 << FILETYPE_SHIFT)

//FIFO special.
#define S_IFIFO  (1 << FILETYPE_SHIFT)

//Regular.
#define S_IFREG  (8 << FILETYPE_SHIFT)

//Directory.
#define S_IFDIR  (4 << FILETYPE_SHIFT)

//Symbolic link.
#define S_IFLNK  (10 << FILETYPE_SHIFT)

//Socket. 
#define S_IFSOCK (12 << FILETYPE_SHIFT)


//Test for a block special file.
#define S_ISBLK(m)  (((m) & S_IFMT) == S_IFBLK)

//Test for a character special file.
#define S_ISCHR(m)  (((m) & S_IFMT) == S_IFCHR)

//Test for a directory.
#define S_ISDIR(m)  (((m) & S_IFMT)  == S_IFDIR)

//Test for a pipe or FIFO special file.
#define S_ISFIFO(m) (((m) & S_IFMT) == S_IFIFO)

//Test for a regular file.
#define S_ISREG(m)  (((m) & S_IFMT) ==  S_IFREG)

//Test for a symbolic link.
#define S_ISLNK(m)  (((m) & S_IFMT) == S_IFLNK)

//Test for a socket.
#define S_ISSOCK(m) (((m) & S_IFMT) ==  S_IFSOCK)

//Set-user-ID on execution.
#define S_ISUID (1 << 9)
//Set-group-ID on execution.
#define S_ISGID (1 << 10)
//On directories, restricted deletion flag. 
#define S_ISVTX (1 << 11)

//File mode bits:

//Read, write, execute/search by owner.
#define S_IRWXU  (7 << 6)

//Read permission, owner.
#define S_IRUSR  (4 << 6)

//Write permission, owner.
#define S_IWUSR  (2 << 6)

//Execute/search permission, owner.
#define S_IXUSR  (1 << 6)


//Read, write, execute/search by group.
#define S_IRWXG  (7 << 3)   

//Read permission, group.
#define S_IRGRP  (4 << 3)

//Write permission, group.
#define S_IWGRP  (2 << 3)

//Execute/search permission, group.
#define S_IXGRP  (1 << 3)


//Read, write, execute/search by others.
#define S_IRWXO  (7 << 0)

//Read permission, others.
#define S_IROTH  (4 << 0)

//Write permission, others.
#define S_IWOTH  (2 << 0)

//Execute/search permission, others.
#define S_IXOTH  (1 << 0)

struct stat {

	dev_t     st_dev;     ///< Device ID of device containing file. 
	ino_t     st_ino;     ///< File serial number. 
	mode_t    st_mode;    ///< Mode of file (see below). 
	nlink_t   st_nlink;   ///< Number of hard links to the file. 
	uid_t     st_uid;     ///< User ID of file. 
	gid_t     st_gid;     ///< Group ID of file. 
                              
	dev_t     st_rdev;    ///< Device ID (if file is character or block special). 

	off_t     st_size;    /* For regular files, the file size in bytes. 
	                        For symbolic links, the length in bytes of the 
	                        pathname contained in the symbolic link.  */
	time_t    st_atime;   ///< Time of last access. 
	time_t    st_mtime;   ///< Time of last data modification. 
	time_t    st_ctime;   ///< Time of last status change. 
                                 
                              
	blksize_t st_blksize; /* A file system-specific preferred I/O block size for 
	                       this object. In some file system types, this may 
	                       vary from file to file.  */

	blkcnt_t  st_blocks;  ///< Number of blocks allocated for this object.
};                            

int    chmod(const char *, mode_t);
int    mkdir(const char *, mode_t);
int    stat(const char *restrict, struct stat *restrict);
int    fchmod(int, mode_t);
int    fstat(int, struct stat *);
int    lstat(const char *restrict, struct stat *restrict);
int    mkfifo(const char *, mode_t);
int    mknod(const char *, mode_t, dev_t);
mode_t umask(mode_t);

#endif                       
