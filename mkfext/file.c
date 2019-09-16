#include "inode.h"
#include "dir.h"
#include "pathparse.h"
#include "block.h"
#include "stat.h"
#include "super_block.h"

#include "local.h"

struct fext_inode_m *file_create(const char *path)
{
	char filename[128];
	char dirname[128];
	split_path(path, filename, dirname);
	struct fext_inode_m *par_i = get_last_dir(dirname);

	if(par_i == NULL){
		return NULL;
	}


	struct fext_inode_m *m_inode = NULL;
	if(!(m_inode = inode_alloc(par_i->fs))){

		//内存分配失败，回滚
		printk("file_create: kmalloc for inode failed\n");
		return NULL;
	}

	uint32_t i_no = m_inode->i_no;
	//申请并安装目录项
	struct fext_dirent dir_entry;
	init_dir_entry(filename, m_inode->i_no, S_IFREG, &dir_entry);

	if(!add_dir_entry(par_i, &dir_entry)){

		printk("add_dir_entry: sync dir_entry to disk failed\n");
		inode_release(m_inode);
		return NULL;
	}

	//磁盘同步两个inode
	inode_sync(par_i);
	inode_sync(m_inode);
	//记得释放

	inode_close(par_i);
	inode_release(m_inode);

	if((m_inode = inode_open(root_fs, i_no)) == NULL){

		printk("open inode failed\n");
		return NULL;
	}
	return m_inode;
}

int32_t inode_write(struct fext_inode_m *m_inode, const void *buf, uint32_t count)
{

	uint32_t blk_nr = 0;
	uint32_t res = count;
	struct fext_fs *fs = m_inode->fs;
	uint32_t block_size = m_inode->fs->sb->block_size;
	struct buffer_head *bh = NULL;
	uint32_t to_write = 0;
	uint32_t blk_idx = 0;
	uint8_t *src = (uint8_t *)buf;

	while(res){
		if(res > block_size){
			to_write = block_size;
			res -= to_write;
		}else {
			to_write = res;
			res = 0;
		}
		
		blk_nr = get_block_num(m_inode, blk_idx, M_CREATE);
		bh = new_buffer_head(fs, blk_nr);

		memcpy(bh->data, src, to_write);

	       	write_block(fs, bh);	
		release_block(bh);

		src += to_write;
		//第一次循环时有用
		++blk_idx;
	}

	m_inode->i_size = count;
	m_inode->i_blocks = DIV_ROUND_UP(m_inode->i_size, block_size);

	inode_sync(m_inode);
	return count;
}

