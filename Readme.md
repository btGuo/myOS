### myos

#### 这只是一个玩具项目，采用宏内核，目前还很垃圾，硬件支持基本为0



#### 目录结构描述

- boch 		//bochs 相关配置文件主要是bochsrc
- device      //设备文模块
- fs               //文件系统模块
- include   //头文件(.h)
- lib              //库文件，分为用户库和内核库，部分文件是相同的
  - kernel
  - user
- test            //一些测试函数，不应该被链接进内核
- thread     //内核线程
- userprog //用户进程
- makefile   
- shell          //shell相关文件，目前还是空的
- Doxyfile   //doxygen配置文件

#### 环境依赖

- bochs         //虚拟机，程序运行环境
- linux           //开发平台
- gcc              //c编译器
- nasm         //汇编器，intel语法好看点
- doxygen   //生成文档

#### 一些环境配置

bochs 安装，ubuntu下

```
sudo apt-get install bochs bochs-x bochs-sdl
```

linux生成磁盘文件，文件名及大小自己配置，系统采用了两个磁盘镜像文件，hd60作为系统盘，hd90作为从盘，存放文件系统。

```
dd if=/dev/zero of=name.img bs=512 count=n
```

格式化从盘

```
sudo fdisk name.img
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

TODO

- 改用gnu汇编
- 采用grub2引导操作系统
- 考虑用uefi替代传统bios，这个应该会很麻烦
- 添加硬件驱动，好多好多....
- 添加信号系统
- 考虑采用scons替代make，可实现跨平台
