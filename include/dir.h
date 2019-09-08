#ifndef __FS_DIR_H
#define __FS_DIR_H

#define MAX_FILE_NAME_LEN 24   ///< 文件名长度，这里主要是为了凑齐字节数

#include "inode.h"
#include "fs.h"
#include "list.h"
#include "dirent.h"

/**
 * 目录项，保存在磁盘，32字节
 */
struct fext_dirent{
	char filename[MAX_FILE_NAME_LEN];   ///< 文件名
	uint32_t f_type;    ///< 文件类型
	uint32_t i_no;      ///< i节点号
};

struct dirent_info {
	char filename[MAX_FILE_NAME_LEN];
	uint32_t f_type;
	uint32_t i_no;
	struct fext_inode_m *m_inode;
	struct list_head hash_tag;
	struct list_head parent_tag;
	struct list_head child_list;
	struct dirent_info  *parent;
};

#define MAX_PATH_LEN 128
#define M_SEARCH 0
#define M_CREATE 1


void dirent_info_init(struct dirent_info *dire_i);
struct Dir* dir_open(struct fext_fs *fs, uint32_t i_no);
struct Dir* dir_new(struct fext_inode_m *inode);
void dir_close(struct Dir *dir);
bool dir_read(struct Dir *dir, struct fext_dirent *dir_e);
void init_dir_entry(char *filename, uint32_t i_no, uint32_t f_type,struct fext_dirent *dir_e);
bool _search_dir_entry(struct fext_inode_m *m_inode, const char *name, struct fext_dirent *dir_e);

bool search_dir_by_ino(struct fext_inode_m *m_inode, uint32_t i_no, struct fext_dirent *dir_e);
struct fext_inode_m *get_last_dir(const char *path);

struct fext_inode_m *search_dir_entry(const char *path, struct fext_dirent *dir_e);
bool add_dir_entry(struct fext_inode_m *inode, struct fext_dirent *dir_e);
bool delete_dir_entry(struct fext_inode_m *inode, uint32_t i_no);
#endif
