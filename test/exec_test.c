#include "stdint.h"
#include "ide.h"
#include "global.h"
#include "fs_sys.h"
#include "process.h"
#include "interrupt.h"
#include "syscall.h"
#include "file.h"

#define filesz 10932
const char *filename = "/dsfogh";

bool prepare(){

	uint32_t secs = DIV_ROUND_UP(filesz, 512);
	struct disk *sda = &channels[0].devices[0];
	char *buf = kmalloc(filesz);
	ide_read(sda, 300, buf, secs);
	int32_t fd = sys_open(filename, O_CREAT | O_RDWR);
	if(fd == -1){
		printk("failed\n");
		return false;
	}

	if(sys_write(fd, buf, filesz) == -1){
		printk("sys write falied\n");
		return false;
	}
	kfree(buf);
	sys_close(fd);	
	//sync();
	printk("prepare done\n");
	return true;
}

void func(){
	const char *argv[] = {"we", "are", "arg"};
	execv(filename, argv);
}

void test_exec(){
	if(!prepare())
		return;
	process_execute(func, "func");
	intr_enable();
}

	
