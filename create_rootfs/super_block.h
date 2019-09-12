#ifndef __FS_SUPERBLOCK_H
#define __FS_SUPERBLOCK_H

#define EXT2_SUPER_MAGIC 0x89896666
#define MAJOR 1
#define MINOR 0

/**
 * 超级块描述符
 */
struct fext_super_block{

	uint32_t magic; ///< 魔数签名

	uint16_t major_rev;  ///< 主版本号
	uint16_t minor_rev;  ///< 次版本号

	uint32_t block_size;        ///< 块大小
	uint32_t blocks_per_group;   ///< 每组块数量
	uint32_t inodes_per_group;  ///< 每组索引节点数
	uint32_t res_blocks;         ///< 剩余块数

	uint32_t blocks_count;        ///< 块总数
	uint32_t inodes_count;        ///< 索引节点总数
	uint32_t free_blocks_count; ///< 空闲块数
	uint32_t free_inodes_count; ///< 空闲索引节点数

	uint32_t create_time;   ///< 创建时间
	uint32_t mount_time;    ///< 挂载时间
	
	uint16_t mount_count;    ///< 挂载次数
	uint32_t last_check;    ///< 最后一次检验时间

	uint32_t groups_table;  ///< 组所在块号
	uint8_t  pad[964];     ///< 凑齐1024(BLOCK_SIZE)字节
};

#endif        
