#ifdef __TEST

#include <kernelio.h>
#include <fs_sys.h>
#include <fs.h>
#include <memory.h>
#include <dir.h>
#include <string.h>
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
	sync_fext();
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
	sync_fext();
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
	sync_fext();
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

void test_dir(){
	DIR *root = sys_opendir("/");
	struct dirent *de = NULL;
	while((de = sys_readdir(root))){
		printk("%s %d;\t", de->filename, de->i_no);
	}
	sys_closedir(root);
	printk("\n");

	int fd = -1;
	if((fd = sys_open("/etc/passwd", O_RDONLY)) == -1){
		printk("open passwd failed\n");
		return;
	}

	char buf[512];
	memset(buf, 0, 512);
	int bytes = 0;
	bytes = sys_read(fd, buf, 20);
	printk("read %d(byte) from %d\n", bytes, fd);
	printk("content %s\n", buf);

	if(sys_mkdir("/test_dir") == -1){
		printk("mkdir error\n");
		return;
	}else {
		printk("mkdir success\n");
	}
	int buf_size = 64;
	char buf[buf_size];
	if(sys_getcwd(buf, buf_size) == NULL){
		printk("getcwd error\n");
		return;
	}else {
		printk("getcwd success\n");
	}
	printk("%s\n", buf);



	if(sys_chdir("/test_dir") == -1){
		printk("chdir error\n");
		return;
	}else {
		printk("chdir success\n");
	}

	if(sys_getcwd(buf, buf_size) == NULL){
		printk("getcwd error\n");
		return;
	}
	printk("%s\n", buf);



	if(sys_mkdir("/test_dir/sub_dir") == -1){
		printk("mkdir error\n");
		return;
	}else {
		printk("mkdir success\n");
	}

	if(sys_chdir("/test_dir/sub_dir") == -1){
		printk("chdir error\n");
		return;
	}else {
		printk("chdir success\n");
	}

	
	if(sys_getcwd(buf, buf_size) == NULL){
		printk("getcwd error\n");
		return;
	}
	
	printk("%s\n", buf);
}

void test_fs(){
	//file_prepare();
	//verify();
	//test_write();
	//w_vefiry();
	//test_writelong();
	//wl_verify();
	test_dir();
}
#endif
