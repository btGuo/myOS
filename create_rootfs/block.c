#include "block.h"
#include "fs.h"
#include "group.h"
#include "bitmap.h"
#include "super_block.h"

#include "local.h"


//下面这组内联名称有点混乱，个人还不知道要起什么名字

/**
 * 一次间接块掩码
 */
static inline uint32_t block_mask1(struct fext_fs *fs)
{
	return ((1 << fs->order) - 1);
}

/**
 * 二次间接块掩码
 */
static inline uint32_t block_mask2(struct fext_fs *fs)
{
	return (block_mask1(fs) << fs->order);
}

/**
 * 三次间接块掩码
 */
static inline uint32_t block_mask3(struct fext_fs *fs)
{
	return (block_mask2(fs) << fs->order);
}

/**
 * 在一次间接块中的索引
 */
static inline uint32_t BLK_IDX_1(struct fext_fs *fs, uint32_t x)
{
	return (x & block_mask1(fs));
}

/**
 * 在二次间接块中的索引
 */
static inline uint32_t BLK_IDX_2(struct fext_fs *fs, uint32_t x)
{
	return ((x & block_mask2(fs)) >> fs->order);
}

/**
 * 在三次间接块中的索引
 */
static inline uint32_t BLK_IDX_3(struct fext_fs *fs, uint32_t x)
{
	return ((x & block_mask3(fs)) >> fs->order >> fs->order);
}

/**
 * 根据间接次数返回块中索引
 */
static inline uint32_t BLK_IDX_I(struct fext_fs *fs, uint32_t x, uint32_t i)
{
	return (i == 1 ? BLK_IDX_1(fs, x):
		i == 2 ? BLK_IDX_2(fs, x):
		BLK_IDX_3(fs, x)) * 4;
}

/**
 * 返回块号对应的级别，一次间接，二次间接或者三次间接
 */
static inline uint32_t BLK_LEVEL(struct fext_fs *fs, uint32_t idx)
{
	return idx < fs->s_indirect_blknr ? 1 :
		idx < fs->d_indirect_blknr ? 2 : 3;
}

/**
 * 计算相对块索引
 */
static inline uint32_t BLK_IDX(struct fext_fs *fs, uint32_t idx)
{
	return idx - (idx < fs->s_indirect_blknr ? 
			fs->direct_blknr :
			idx < fs->d_indirect_blknr ? 
				fs->s_indirect_blknr :
				fs->d_indirect_blknr
		);
}
/**
 * 分配block块
 *
 * @return 新分配块的块号
 * 	@retval -1 分配失败
 */
int32_t block_bmp_alloc(struct fext_fs *fs){
	struct fext_group_m *cur_gp = fs->cur_gp;
	struct fext_super_block *sb = fs->sb;

	if(sb->free_inodes_count <= 0)
		return -1;
	//当前组中已经没有空闲位置，切换到下一个组
	if(cur_gp->free_blocks_count == 0){

		cur_gp = group_switch(fs);
	}
	//注意减1
	--cur_gp->free_blocks_count;
	--sb->free_blocks_count;

	uint32_t idx = bitmap_scan(&cur_gp->block_bmp, 1);
	bitmap_set(&cur_gp->block_bmp, idx, 1);
	//这里要加上一块引导块
	return idx + cur_gp->group_nr * fs->sb->blocks_per_group;
}

/**
 * 在块位图中复位
 */

void block_bmp_clear(struct fext_fs *fs, uint32_t blk_nr){
	//TODO gp位图可能不在内存中
	struct fext_group_m *gp = fs->groups + blk_nr / fs->sb->blocks_per_group;

	blk_nr %= fs->sb->blocks_per_group;
	bitmap_set(&gp->block_bmp, blk_nr, 0);
}

//将文件系统块号转换为磁盘扇区号
/**
 * 磁盘直接写
 */
void write_direct(struct fext_fs *fs, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	sta_blk_nr *= fs->sec_per_blk;
	cnt *= fs->sec_per_blk;
	//这里要加上分区起始lba
	ide_write(fs->part->disk, fs->part->start_lba + sta_blk_nr, data, cnt);
}	

/**
 * 磁盘直接读
 */
void read_direct(struct fext_fs *fs, uint32_t sta_blk_nr, void *data, uint32_t cnt){
	sta_blk_nr *= fs->sec_per_blk;
	cnt *= fs->sec_per_blk;
	//这里要加上分区起始lba
	ide_read(fs->part->disk, fs->part->start_lba + sta_blk_nr, data, cnt);
}

/**
 * @brief 写磁盘块
 * @note 操作的对象为buffer_head
 */
void write_block(struct fext_fs *fs, struct buffer_head *bh){
	if(bh->is_buffered){
		BUFW_BLOCK(bh);
		return;
	}
	write_direct(fs, bh->blk_nr, bh->data, 1);
}

/**
 * @brief 读取磁盘块
 * @detail 先在缓冲区中查找，命中则直接返回，否则从磁盘读取块并尝试加入
 * 缓冲区，这里并没有设置脏位
 *
 * @param blk_nr 块号
 */
struct buffer_head *read_block(struct fext_fs *fs, uint32_t blk_nr){
	struct buffer_head *bh = buffer_read_block(&fs->io_buffer, blk_nr);
	if(bh){
		//printk("buffer hit\n");
		return bh;
	}

	uint32_t block_size = fs->sb->block_size;
	bh = (struct buffer_head *)kmalloc(sizeof(struct buffer_head));
	if(!bh){
		PANIC("no more space\n");
	}
	bh->data = (uint8_t *)kmalloc(block_size);
	if(!bh->data){
		PANIC("no more space\n");
	}

	bh->blk_nr = blk_nr;
	bh->lock = true;
	bh->is_buffered = true;
//	bh->dirty = true;
	//从磁盘读
	read_direct(fs, blk_nr, bh->data, 1);
	//加入缓冲区
	if(!buffer_add_block(&fs->io_buffer, bh)){
		//缓冲区已经满了
		bh->is_buffered = false;
	}
	return bh;
}

struct buffer_head *new_buffer_head(struct fext_fs *fs, uint32_t blk_nr){

	struct buffer_head *bh = 
		(struct buffer_head *)kmalloc(sizeof(struct buffer_head));

	if(!bh){
		return NULL;
	}
	bh->data = (uint8_t *)kmalloc(fs->sb->block_size);
	if(!bh->data){
		kfree(bh);
		return NULL;
	}

	bh->blk_nr = blk_nr;
	bh->lock = true;
	bh->is_buffered = buffer_add_block(&fs->io_buffer, bh) ? true : false;

	return bh;
}



/**
 * 释放内存中的块
 */
void release_block(struct buffer_head *bh){
	if(bh->is_buffered){
		BUFR_BLOCK(bh);
		return;
	}
	kfree(bh->data);
	kfree(bh);
}

/**
 * 将块号为blk_nr的块清零
 *
 * @param blk_nr 块号
 */
void init_block(struct fext_fs *fs, uint32_t blk_nr){

	uint32_t block_size = fs->sb->block_size;
	struct buffer_head *bh = read_block(fs, blk_nr);
	memset(bh->data, 0, block_size);
	//同步磁盘
	write_block(fs, bh);
	release_block(bh);
}

//有待debug
/**
 * 将文件内的块号转换为实际块号
 *
 * @param fs 分区指针
 * @param inode i节点指针
 * @param idx  该i节点中第idx个块
 * @param mode M_SEARCH 或者 M_CREATE 搜索或者创建，
 * 如果为创建数据块不在时则会新分配块号
 *
 * @return 物理块号
 * 	@retval 0 查找失败
 */
uint32_t get_block_num(struct fext_inode_m *inode, uint32_t idx, uint8_t mode){

	struct fext_fs *fs = inode->fs;
	if(idx >= fs->max_blocks){
		PANIC("no more space\n");
	}
	
	struct buffer_head *bh = NULL;
	uint32_t blk_nr = 0;

	//直接块
	if(idx < fs->direct_blknr){
		blk_nr = inode->i_block[idx];
		if(!blk_nr){

			//搜索为空，直接返回
			if(mode == M_SEARCH)
				return 0;

			uint32_t new_blk_nr = block_bmp_alloc(inode->fs);
			inode->i_block[idx] = new_blk_nr;
			blk_nr = new_blk_nr;
			//inode_sync(inode);
		}
	//间接块
	}else {

		idx = BLK_IDX(inode->fs, idx);
		//间接次数
		uint32_t cnt = BLK_LEVEL(inode->fs, idx);

		blk_nr = inode->i_block[fs->direct_blknr - 1 + cnt];
		//基地址不存在，单独处理
		if(!blk_nr){

			uint32_t new_blk_nr = block_bmp_alloc(inode->fs);
			inode->i_block[fs->direct_blknr - 1 + cnt] = new_blk_nr;
			blk_nr = new_blk_nr;
			init_block(inode->fs, blk_nr);		
		}

		uint32_t i = 1;
		uint32_t *pos = NULL;
		while(i <= cnt){

			bh = read_block(inode->fs, blk_nr);
			pos = (uint32_t *)&bh->data[BLK_IDX_I(inode->fs, idx, i)];
			blk_nr = *pos;

			//块不存在
			if(!blk_nr){
				//最后一块为数据块
				if(i == cnt && mode == M_SEARCH)
					return 0;
				uint32_t new_blk_nr = block_bmp_alloc(inode->fs);

				*pos = new_blk_nr;
				//写入硬盘
				write_block(inode->fs, bh);
				blk_nr = new_blk_nr;
				//attention 第一次申请索引块必须先清0，数据块不需要
				if(i != cnt)
					init_block(inode->fs, blk_nr);		
			}

			//释放
			release_block(bh);
			++i;
		}
	}	
	return blk_nr;
}
