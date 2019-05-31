#ifndef __FS_FS_H
#define __FS_FS_H

#define MAX_FILES_PER_PART 4096
#define SECTOR_SIZE 512
#define BLOCK_SIZE 512
//每个块有多少个扇区
#define SEC_PER_BLK (BLOCK_SIZE / SECTOR_SIZE)
//每个扇区多少位
#define BITS_PER_SEC (SECTOR_SIZE * 8)

enum file_types{
	FT_UNKNOWN;
	FT_REGULAR;
	FT_DIRECTORY;
};

void filesys_init();

#endif
