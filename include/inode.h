#ifndef __FS_INODE_H
#define __FS_INODE_H

#include "stdint.h"
#include "list.h"

struct inode{
	uint32_t i_no;
	uint32_t i_size;
	uint32_t i_open_cnts;
	uint32_t i_sectors[13];
	bool write_deny;
	struct list_head inode_tag;
};

#endif
