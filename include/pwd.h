#ifndef __SYS_PWD_H
#define __SYS_PWD_H

#include <sys/types.h>

#define PW_NAMESZ   64
#define PW_DIRSZ    128
#define PW_PASSWDSZ 128

struct passwd {

	char *pw_name;   ///< 用户名
	uid_t pw_uid;   ///< 用户id
	git_t pw_gid;   ///< 组id
	char *pw_dir;   ///< 主目录
	char *pw_passwd; ///< 密码，密文
};
	
struct passwd *getpwuid(uid_t uid);

#endif
