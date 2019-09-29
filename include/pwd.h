#ifndef __SYS_PWD_H
#define __SYS_PWD_H

#include <sys/types.h>

#define PW_NAMESZ   64
#define PW_DIRSZ    128
#define PW_PASSWDSZ 128

struct passwd {

	char *pw_name;   ///< 用户名
	char *pw_passwd; ///< 密码
	uid_t pw_uid;   ///< 用户id
	gid_t pw_gid;   ///< 组id
	char *pw_info;  ///< 用户其他信息   
	char *pw_dir;   ///< 主目录
	char *pw_shell;  ///< shell
};
	
struct passwd *getpwuid(uid_t uid);
struct passwd *getpwnam(const char *);

int getpwnam_r(const char *, struct passwd *, char *, size_t, struct passwd **);
int getpwuid_r(uid_t, struct passwd *, char *, size_t, struct passwd **);
#endif
