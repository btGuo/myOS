#ifndef __FS_DIR_H
#define __FS_DIR_H

#define MAX_FILE_NAME_LEN 24   ///< 文件名长度，这里主要是为了凑齐字节数

/**
 * 目录，用于内存
 */
struct dir{
	struct inode * inode;
	uint32_t dir_pos;
	uint8_t dir_buf[512];
};

/**
 * 目录项，保存在磁盘
 */
struct dir_entry{
	char filename[MAX_FILE_NAME_LEN];
	enum file_types f_type;    ///< 文件或目录
	uint32_t i_no;      ///< i节点号
};

#endif
