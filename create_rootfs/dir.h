#ifndef __FS_DIR_H
#define __FS_DIR_H

#define MAX_FILE_NAME_LEN 24   ///< 文件名长度，这里主要是为了凑齐字节数

#include "inode.h"
#include "fs.h"

#include "local.h"

/**
 * 目录项，保存在磁盘，32字节
 */
struct fext_dirent{
	char filename[MAX_FILE_NAME_LEN];   ///< 文件名
	uint32_t f_type;    ///< 文件类型
	uint32_t i_no;      ///< i节点号
};

#define MAX_PATH_LEN 128
#define M_SEARCH 0
#define M_CREATE 1


void init_dir_entry(char *filename, uint32_t i_no, uint32_t f_type,struct fext_dirent *dir_e);
bool _search_dir_entry(struct fext_inode_m *m_inode, const char *name, struct fext_dirent *dir_e);

bool search_dir_by_ino(struct fext_inode_m *m_inode, uint32_t i_no, struct fext_dirent *dir_e);
struct fext_inode_m *get_last_dir(const char *path);

struct fext_inode_m *search_dir_entry(const char *path, struct fext_dirent *dir_e);
bool add_dir_entry(struct fext_inode_m *inode, struct fext_dirent *dir_e);
int32_t sys_mkdir(char *path);
#endif
