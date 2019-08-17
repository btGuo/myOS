#include "debug.h"
#include "fs_sys.h"
#include "fs.h"
#include "memory.h"
#include "data.h"

const uint32_t sz = 2384;
const char *name = "/sdfii";
const char ch = 'a';
const uint32_t off = 123;
const char *key = "i am a key";
const uint32_t key_len = 11;

void file_prepare(){

	int32_t fd = sys_open(name, O_CREAT | O_RDWR);
	if(fd == -1){
		printk("error\n");
		return;
	}
	char *buf = kmalloc(sz);
	uint32_t i = sz;
	while(i--){
		buf[i] = ch;
	}
	if(sys_write(fd, buf, sz) == -1){
		printk("error\n");
		return;
	}
	kfree(buf);
	sys_close(fd);
	sync();
	printk("done\n");
}

void verify(){
	int32_t fd = sys_open(name, O_RDWR);
	if(fd == -1){
		printk("error\n");
		return;
	}
	char *buf = kmalloc(sz);
	
	if(sys_read(fd, buf, sz) == -1){
		printk("error\n");
		return;
	}

	uint32_t i = sz;
	while(i--){
		if(buf[i] != ch){
			printk("buf wrong\n");
			break;
		}
	}
	kfree(buf);
	sys_close(fd);
	printk("right\n");
}	

void test_write(){
	
	int32_t fd = sys_open(name, O_RDWR);
	if(fd == -1){
		printk("error\n");
		return;
	}
	sys_lseek(fd, off, SEEK_SET);
	if(sys_write(fd, key, key_len) == -1){
		printk("error\n");
		return;
	}
	sys_close(fd);
	sync();
	printk("done\n");
}

void w_vefiry(){
	int32_t fd = sys_open(name, O_RDWR);
	if(fd == -1){
		printk("error\n");
		return;
	}
	char buf[100];
	
	sys_lseek(fd, off, SEEK_SET);
	if(sys_read(fd, buf, key_len) == -1){
		printk("error\n");
		return;
	}
	uint32_t i = 0;
	while(i < key_len){
		if(buf[i] != key[i]){
			printk("buf wrong\n");
			break;
		}
		i++;
	}
	sys_close(fd);
	printk("done\n");
}

void test_writelong(){
	int32_t fd = sys_open(name, O_RDWR);
	if(fd == -1){
		printk("error\n");
		return;
	}
	if(sys_write(fd, data, len) == -1){
		printk("error\n");
		return;
	}
	sys_close(fd);
	sync();
	printk("done\n");
}

void wl_verify(){

	int32_t fd = sys_open(name, O_RDWR);
	if(fd == -1){
		printk("error\n");
		return;
	}
	char *buf = kmalloc(len);
	if(sys_read(fd, buf, len) == -1){
		printk("error\n");
		return;
	}
	uint32_t i = 0;
	while(i < len){
		if(buf[i] != data[i]){
			printk("index %d buf wrong\n", i);
			break;
		}
		i++;
	}
	sys_close(fd);
	printk("done\n");
}

void test_fs(){
	//file_prepare();
	//verify();
	//test_write();
	//w_vefiry();
	//test_writelong();
	wl_verify();
}
