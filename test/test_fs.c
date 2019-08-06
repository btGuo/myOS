#include "print.h"
#include "fs_sys.h"
#include "fs.h"

//filename qqqq awqq bwqq

void test_fs(){

	int fd = sys_open("/dwqq", O_CREAT | O_RDWR);

	char buf[1024];
	int i = 1024;
	while(i--)
		buf[i] = 'i';
	i = 1;
	printk("here\n");
	while(i--)
		sys_write(fd, buf, 1024);

	printk("here\n");
	sys_close(fd);
	
	printk("here\n");

	if(sys_unlink("/dwqq") == -1){
		printk("error");
	}
	printk("here\n");

	// /dir1 /dir1/a  /dir1/b  /dir1/fa 

	fd = sys_open("/dir1/fa", O_CREAT | O_RDWR);
	sys_close(fd);

	struct dir *dir = sys_opendir("/dir1");
	if(!dir){
		printk("open failed\n");
	}
	struct dir_entry dir_e;
	while(sys_readdir(dir, &dir_e) != -1){
		printk("dir_e.filename %s    ", dir_e.filename);
	}
	printk("\n");
	sync();

}
