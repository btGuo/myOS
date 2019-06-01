#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"

typedef long time_t

#define EXT2_N_BLOCKS 15 ///< 12个直接，1个间接，1个双间接，1个三间接

struct inode{
	time_t i_ctime; ///< 创建时间
	time_t i_mtime; ///< 修改时间
	time_t i_atime; ///< 访问时间
	uint16_t i_uid; ///< 用户id
	uint16_t i_gid;  ///< 用户组id
	uint16_t i_mode; ///< 访问模式
	uint16_t i_links_count  ///< 链接次数
	uint32_t i_no;  ///< i节点号
	uint32_t i_size; ///< 大小
	uint32_t i_sectors[EXT2_N_BLOCKS];

	uint32_t i_open_cnts;
	bool write_deny;
	struct list_head inode_tag;
	uint32_t zeros[6];  ///< 凑齐字节数
};

#endif
