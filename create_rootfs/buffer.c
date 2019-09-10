#include "buffer.h"
#include "fs.h"

struct buffer_head *buffer_read_block(struct disk_buffer *d_buf, uint32_t blk_nr){
	return NULL;
}

struct fext_inode_m *buffer_read_inode(struct disk_buffer *d_buf, uint32_t i_no){
	return NULL;
}


bool buffer_add_block(struct disk_buffer *d_buf, struct buffer_head *bh){
	return false;
}

bool buffer_add_inode(struct disk_buffer *d_buf, struct fext_inode_m *m_inode){
	return false;
}

void buffer_sync(struct disk_buffer *d_buf){
	/** empty */
}

void disk_buffer_init(struct disk_buffer *d_buf, struct fext_fs *fs){
	/** empty */
}
