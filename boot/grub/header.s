
.set ALIGN, 1<<0 
.set MEMINFO, 1<<1
.set FLAGS,  ALIGN|MEMINFO
.set MAGIC,  0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.section .multiboot
.align 8
.long MAGIC
.long FLAGS
.long CHECKSUM

gdt:
	.quad 0
	.quad 0x00cf98000000ffff
	.quad 0x00cf92000000ffff
	.quad 0x00cf920b80000007

.set GDT_SIZE .-gdt
.set GDT_LIMIT GDT_SIZE - 1

.set SELECTOR_CODE  0x0008
.set SELECTOR_DATA  0x0010
.set SELECTOR_VIDEO 0x0018

	.fill 60, 8, 0

total_mem_bytes:
	.long 0

gdt_ptr:
	.word GDT_LIMIT
	.long gdt

ards_buf:
	.fill 244, 1, 0
ards_nr:
	.word 0

.code16
.global _start
.section .text
_start:
	xorl %ebx, %ebx
	movl $0x534d4150 %edx
	movw $ards_buf %di

.e820_mem_get:
	movl $0x000e820 %eax
