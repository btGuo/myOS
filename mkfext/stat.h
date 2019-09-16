#ifndef __STAT_H
#define __STAT_H

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
#define S_IRWXU  (7 << 16)

//Read permission, owner.
#define S_IRUSR  (4 << 16)

//Write permission, owner.
#define S_IWUSR  (2 << 16)

//Execute/search permission, owner.
#define S_IXUSR  (1 << 16)


//Read, write, execute/search by group.
#define S_IRWXG  (7 << 8)   

//Read permission, group.
#define S_IRGRP  (4 << 8)

//Write permission, group.
#define S_IWGRP  (2 << 8)

//Execute/search permission, group.
#define S_IXGRP  (1 << 8)


//Read, write, execute/search by others.
#define S_IRWXO  (7 << 0)

//Read permission, others.
#define S_IROTH  (4 << 0)

//Write permission, others.
#define S_IWOTH  (2 << 0)

//Execute/search permission, others.
#define S_IXOTH  (1 << 0)

#endif                       
