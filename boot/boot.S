#include "boot.h"
# Declare constants for the multiboot header.
.set ALIGN,    1<<0             # align loaded modules on page boundaries
.set MEMINFO,  1<<1             # provide memory map
.set FLAGS,    ALIGN | MEMINFO  # this is the Multiboot 'flag' field
.set MAGIC,    0x1BADB002       # 'magic number' lets bootloader find the header
.set CHECKSUM, -(MAGIC + FLAGS) # checksum of above, to prove we are multiboot
.set MULTIBOOT_BOOTLOADER_MAGIC, 0x2BADB002

.set STACK_SIZE, 0x400
.set PAGE_SIZE, 0x1000
.set KERNEL_STACK, 0xc009f000

.set KERNEL_PAGE_DIR, 0x00200000
.set KERNEL_PAGE_TABLE, 0x00201000

.set SELECTOR_CODE, 0x0008
.set SELECTOR_DATA, 0x0010
.set SELECTOR_VIDEO, 0x0018

# Declare a header as in the Multiboot Standard.
.section .multiboot
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .text
.code32
.global _start
.type _start, @function
_start:
	#验证头部
	cmpl $MULTIBOOT_BOOTLOADER_MAGIC, %eax
	jnz halt

	cli

	#加载gdt
	movl $gdtptr, %eax
	lgdt (%eax)

	#刷新cs
	ljmp $SELECTOR_CODE, $refresh 

refresh:

	movl $SELECTOR_DATA, %eax
	movl %eax, %ds
	movl %eax, %es
	movl %eax, %fs
	movl %eax, %ss
	movl $stack, %esp
	movl $SELECTOR_VIDEO, %eax
	movl %eax, %gs

	pushl %ebx

#----------------------------------------------

setup_page:
	movl $0x1000, %ecx
	movl $0, %esi

	#清空页目录
clear_page_dir:
	movb $0, KERNEL_PAGE_DIR(%esi)
	incl %esi
	loop clear_page_dir

	#创建页目录项0
	movl $KERNEL_PAGE_TABLE, %eax
	orl $0x7, %eax
	movl %eax, KERNEL_PAGE_DIR

	#低端4M
	movl $0x400, %ecx
	movl $0x7, %edx
	movl $KERNEL_PAGE_TABLE, %ebx
	movl $0, %esi

create_pte:
	movl %edx, (%ebx, %esi, 4)
	incl %esi
	addl $0x1000, %edx
	loop create_pte


	#填充内核页目录
	movl $0x300, %esi
	movl $KERNEL_PAGE_DIR, %ebx

	movl $KERNEL_PAGE_TABLE, %edx
	orl  $0x7, %edx

	#255项
	movl $0xff, %ecx
create_k_pde:

	movl %edx, (%ebx, %esi, 4)
	incl %esi
	addl $0x1000, %edx
	loop create_k_pde

	movl $KERNEL_PAGE_DIR, %eax
	orl  $0x7, %eax
	movl %eax, KERNEL_PAGE_DIR + 4092

#---------------------------------------------

	#重新定位gdt
	orl $0xc0000000, gdtptr + 2

	#cr3 赋值
	movl $KERNEL_PAGE_DIR, %eax
	movl %eax, %cr3

	#开启页模式
	movl %cr0, %eax
	orl  $0x80000000, %eax
	movl %eax, %cr0

	#重新加载gdt
	movl $gdtptr, %eax
	lgdt (%eax)

	#这里拿出来
	popl %ebx
	movl $KERNEL_STACK, %esp
	#更换栈了，重新推入
	pushl %ebx
	call kernel_main
halt:
	cli
	hlt
	jmp halt

.section .bss

#临时栈
.align PAGE_SIZE
stack: .fill STACK_SIZE, 1, 0

.section .data
.align 8
gdt:
	.quad 0
	.quad 0x00cf98000000ffff #code
	.quad 0x00cf92000000ffff #data
	.quad 0x00cf920b80000007 #video
gdtend:

gdtptr:
	.word gdtend - gdt
	.long gdt
	.word 0

