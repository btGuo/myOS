# myos

#### myos是由c语言编写的，一个简单的操作系统宏内核，目前只支持x86_32架构，只能运行在bochs上。

### 各模块的实现

#### 内核内存分配

该部分的代码位于kernel目录下，物理页框的分配采用伙伴系统算法，防止外碎片。内核小对象的分配采用linux内核0.11中的分配方法，将物理页划分成2 ^ n 大小相同的内存块，要分配的内存大小取整到2 ^ n，之后分配。

#### 文件系统

该部分的代码位于fs目录下，实现了一个简化版的ext文件系统。文件系统的布局同ext2布局一样，分为多个块组。根据文件系统块号映射实现了块缓冲。实现了mkfext工具用于格式化文件系统，工具位于目录mkfext下编译后即可使用。

#### 用户进程

进程的内存布局是经典的32位内存布局，0 - 3G为用户内存区，3 - 4G为内核映射内存区域，用户内存依次为代码段，数据段，之后是堆区，栈区，最顶端的是运行时参数。目前可运行的可执行文件格式为elf32格式，fork实现了写时复制，fork时只映射了页表，相应的数据页并没有被改动。

#### 用户进程调度

实现简单的轮转调度，进程被分配固定的时间片，时间片用完的进程会被加入到队列尾部，等待之后的调度。进程可被阻塞和唤醒，组摄时进程移出就绪队列，唤醒时再次加入就绪队列。

#### 数据结构

该部分代码位于lib/kernel下面。

hash_table: 开哈希表，哈希表每个桶由链表组成，key类型可以是int或者字符串类型。

list_head: linux内核2.6链表。

bitmap: 位图数组。

ioqueue: 一个生产者消费者环形队列，生产者或消费者会被阻塞和唤醒。

#### 系统引导

系统采用grub multiboot引导。

#### 系统加载过程

grub加载完系统后，系统进行初始化，系统初始化完成后，会载入/bin/init可执行文件，形成init进程，init进程源代码位在bin/init.c，之后init进程开始初始化终端，fork出子进程执行shell，shell源代码位于bin/shell.c，系统开始运行。

#### 用户态内存分配

即malloc和free的实现，采用了csapp中的隐式空闲链表进行通用内存的分配。该部分代码位于lib/user目录下。

#### 物理内存布局

```
0 - 1M: 低端内存不用。
1 - 2M: 内核镜像。
2 - 3M: 内核页表。
3 - _M: 所有物理页描述符。
_ - 4nM: 零散物理页。
4n - 4mM: 内核固定映射内存，此部分内存由伙伴系统管理。
4mM - _: 用户态内存和内核动态内存分配区。
```

### 实现的posix标准

```
dirent.h:
struct dirent *readdir(DIR *dir);
void   rewinddir(DIR *dir);
int    closedir(DIR *dir);
DIR    *opendir(const char *path);

unistd.h: 
int     close(int);
int     chdir(const char *);
int     dup(int);
int     dup2(int, int);
int     execve(const char *, char *const [], char *const []);
int     access(const char *path, int amode);
pid_t   fork(void);
pid_t   getpid(void);
char    *getcwd(char *, size_t);
off_t   lseek(int, off_t, int);
ssize_t read(int, void *, size_t);
int     unlink(const char *);
ssize_t write(int, const void *, size_t);
void    _exit(int);
int     pipe(int [2]);
uid_t   getuid(void);

sys/utsname.h:
int uname(struct utsname *);

sys/wait.h:
pid_t wait(int *);
pid_t waitpid(pid_t, int *, int);

stdlib.h:
int  atoi(const char *src);
void *malloc(size_t);
void free(void *);
void *realloc(void *, size_t);
void *calloc(size_t lens, size_t size);
void exit(int);

stdio.h:
int printf(const char *fmt, ...);
int vsprintf(char *buf, const char *fmt, va_list);
int sprintf(char *buf, const char *fmt, ...);
int putchar(int);

stat.h:
int    chmod(const char *, mode_t);
int    mkdir(const char *, mode_t);
int    stat(const char *restrict, struct stat *restrict);

fcntl.h:
int open(const char *path, int flag, ...);
```

#### 环境配置

- bochs: 编译器
- nasm: 汇编器
- doxygen: 文档生成工具
- grub: 引导工具

#### 源码获取及编译

```
git clone https://github.com/btGuo/myOS.git
make all
```

编译之后会生成内核镜像，位于build/kernel.bin

#### 虚拟磁盘制作以及格式化

```
#创建虚拟硬盘，64M大小
dd if=/dev/zero of=disk.img bs=512 count=131072

#格式化分区
sudo fdisk disk.img

#可能设备忙，先取消
sudo losetup -d /dev/loop0
sudo losetup -d /dev/loop1
#可能设备忙，先取消挂载
sudo umount /dev/loop0
sudo umount /dev/loop1

#设置回环设备
sudo losetup /dev/loop0 disk.img
sudo losetup /dev/loop1 disk.img -o 1048576

#创建 fat 文件系统
sudo mkdosfs -F32 -f 2 /dev/loop1

#挂载
sudo mount /dev/loop1 /mnt

#安装grub
sudo grub-install --target=i386-pc --root-directory=/mnt \
	--no-floppy --modules="normal part_msdos multiboot" /dev/loop0

#复制内核镜像以及配置文件过去
sudo cp build/kernel.bin /mnt/boot
sudo cp boot/grub.cfg /mnt/boot/grub

#同步磁盘
sync

#取消挂载
sudo umount /mnt

#取消设备
sudo losetup -d /dev/loop1
sudo losetup -d /dev/loop0
```

#### 系统根目录安装

编译mkfext

```
cd mkfext && make
```

创建根目录结构

```
mkdir rootfs
mkdir rootfs/bin
mkdir rootfs/home
mkdir rootfs/etc
```

安装可执行文件到根目录

```
cd ../bin && make all && make install
```

利用mkfext写入磁盘镜像

```
./mkfext ../disk.img -d rootfs
```

#### 参考书籍

- 实践部分
  - 《操作系统真相还原》郑刚著，首要参考书籍，一开始就是照着这本书的教程写的，后来参考linux内核，又改了很多模块

- 汇编知识
  - 《汇编语言第三版》王爽著
  - 《x86汇编语言：从实模式到保护模式》李忠 / 王晓波 / 余洁 著
- linux内核
  - 《linux内核0.11完全注解》赵炯，赵炯博士的网站 : [oldlinux](http://oldlinux.org)
  - 《深入理解linux内核》Daniel P.Bovet、Marco Cesati著，讲linux内核2.6，进阶书籍
- 其他
  - 一个几乎无敌的网站 [osdev](http://osdev.org)上面有各种你想要的答案
  - gnu 各种手册，可在gnu官网上找到 [gnu](http://gnu.org)
  - 一个c++写的微内核项目，值得学习 ：[freenos](http://freenos.org)
