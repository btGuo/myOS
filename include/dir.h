#ifndef __FS_DIR_H
#define __FS_DIR_H

#define MAX_FILE_NAME_LEN 24   ///< 文件名长度，这里主要是为了凑齐字节数

#include "ide.h"
#include "inode.h"
#include "fs.h"
/**
 * 目录，用于内存
 */
struct dir{
	struct inode_info *inode;
	uint32_t dir_pos;
	uint8_t dir_buf[512];
};

/**
 * 目录项，保存在磁盘，32字节
 */
struct dir_entry{
	char filename[MAX_FILE_NAME_LEN];
	enum file_types f_type;    ///< 文件或目录
	uint32_t i_no;      ///< i节点号
};

#define M_SEARCH 0
#define M_CREATE 1

void open_root_dir(struct partition *part);
void dir_close(struct dir *dir);
struct dir* dir_open(struct partition *part, uint32_t i_no);
struct buffer_head *_handle_inode(struct partition *part, struct inode_info *inode,\
	       	uint32_t idx, uint8_t mode);
void create_dir_entry(char *filename, uint32_t i_no, enum file_types f_type,\
		struct dir_entry *dir_e);
bool search_dir_entry(struct partition *part, struct dir *dir, \
	const char *name, struct dir_entry *dir_e);
bool add_dir_entry(struct dir *par_dir, struct dir_entry *dir_e);

void print_root(struct inode_info *m_inode);
#endif
