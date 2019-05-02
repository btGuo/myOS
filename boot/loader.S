%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
;栈位于loader 下方
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start
;全局描述符表
gdt:
	dq 0
	dq 0x00_c_f_9_8_000000_ffff
	dq 0x00_c_f_9_2_000000_ffff
	dq 0x00_c_f_9_2_0b8000_0007

GDT_SIZE  equ $ - gdt
GDT_LIMIT equ GDT_SIZE - 1

SELECTOR_CODE  equ 0x0008
SELECTOR_DATA  equ 0x0010
SELECTOR_VIDEO equ 0x0018

	times 60 dq 0

total_mem_bytes dd 0

gdt_ptr:
	dw GDT_LIMIT
	dd gdt

ards_buf times 244 db 0
ards_nr  dw 0		

loader_start:
;打印字符串
;cx 字符串长度
;dl 指定颜色	
	mov cx, 12
	mov si, message
	mov ax, 0xb800
	mov es, ax
	mov di, 0	
	mov dl, 0xa4
;向显存写入字符	
.s: mov dh, [si]
	inc si
	mov [es: di], dh
	inc di
	mov [es: di], dl
	inc di
	loop .s

;==============================================================
;int 0x15 eax = E820
;第一次调用，ebx置0
	xor ebx, ebx
;注意设置es
	mov ax, cx
	mov es, ax
	mov di, ards_buf
	mov edx, 0x534d4150

.e820_mem_get:
	mov eax, 0x0000e820
	mov ecx, 20
	int 0x15	
	jc .e801_mem_get
	add di, cx
	inc word [ards_nr]
	cmp ebx, 0
	jnz .e820_mem_get
;这里是cx
	mov cx, [ards_nr]
	mov ebx, ards_buf
	xor edx, edx
;找到一个最大项
.find_max_mem:
	mov eax, [ebx]
	add eax, [ebx + 8]
	add ebx, 20
	cmp edx, eax
	jge .next_ards
	mov edx, eax
.next_ards:
	loop .find_max_mem
	jmp .mem_get_ok

;==============================================================
;int 0x15 ax = 0xe801
.e801_mem_get:
	mov ax, 0xe801
	int 0x15
	jc .88_mem_get

	mov cx, 0x400
	mul cx
	shl edx, 16
	and eax, 0x0000ffff
	or  edx, eax
	add eax, 0x100000	
	mov ecx, eax

	mov eax, 0x10000
	and edx, 0x0000ffff
	mul edx
	add eax, ecx
	mov ebx, eax
	jmp .mem_get_ok

;==============================================================
.88_mem_get:
	mov ah, 0x88
	int 0x15
	jc .error_hlt

	and eax, 0x0000ffff
	mov cx, 0x400
	mul cx
	shl edx, 16
	or  edx, eax
	add edx, 0x10000
	jmp .mem_get_ok

;故障，无法找到内存
.error_hlt:
	jmp $

;成功获取内存
.mem_get_ok:
	mov [total_mem_bytes], edx 

;===============================================================
;打开A20	
	in al, 0x92
	or al, 00000010B
	out 0x92, al

;加载gdt
	lgdt [gdt_ptr]

;开启保护模式
	mov eax, cr0
	or  eax, 0x00000001
	mov cr0, eax
;刷新流水线
	jmp dword  SELECTOR_CODE:p_mode_start

[bits 32]
p_mode_start:
	mov ax, SELECTOR_DATA
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov ss, ax
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax

	mov eax, KERNEL_START_SECTOR
	mov ebx, KERNEL_BIN_BASE_ADDR
	mov ecx, 200
	call rd_disk_32

	call setup_page
	sgdt [gdt_ptr]
	mov ebx, [gdt_ptr + 2]
	or  dword [ebx + 0x18 + 4], 0xc0000000

	add dword [gdt_ptr + 2], 0xc0000000
	add esp, 0xc0000000

	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax
;开启页模式
	mov eax, cr0
	or  eax, 0x80000000
	mov cr0, eax

	lgdt [gdt_ptr]

	call kernel_init
	mov esp, 0xc009f000

	jmp KERNEL_ENTRY_POINT

;=====================================================================
;开启分页
setup_page:
;清空页目录
	mov ecx, 0x1000
	mov esi, 0
.clear_page_dir:
	mov byte [PAGE_DIR_TABLE_POS + esi], 0
	inc esi
	loop .clear_page_dir

;创建页目录0项
.create_pde:
	mov eax, PAGE_TABLE_POS
	or  eax, 0x7
	mov [PAGE_DIR_TABLE_POS], eax

;创建页表0项
;指向低端2M空间
;低端1M用于内核
;剩下的1M - 2M用于内核页目录和页表
	mov ecx, 0x200
	mov edx, 0x7
	mov ebx, PAGE_TABLE_POS
	mov esi, 0
.create_pte:
	mov [ebx + esi * 4], edx
	inc esi
	add edx, 0x1000
	loop .create_pte

;从页目录第768项开始
	mov esi, 0x300
	mov ebx, PAGE_DIR_TABLE_POS
;从页表第一项开始
	mov edx, PAGE_TABLE_POS
	or  edx, 0x7
;256项
	mov ecx, 0x100
.create_kernel_pde:
	mov [ebx + esi * 4], edx
	inc esi
	add edx, 0x1000
	loop .create_kernel_pde

	ret

;========================================================================
;读取硬盘
;eax 起始lba地址
;ebx  写入地址
;ecx  读取扇区数
;内容同rd_disk_16
rd_disk_32:
	mov esi, eax
	mov edi, ecx
;第一步：确定通道, 写入扇区数
	mov dx, 0x1f2
	mov al, cl
	out dx, al

	mov eax, esi
;第二步：写入lba地址
;dx 起始端口	
	mov dx, 0x1f3
	mov cx, 3
.s: out dx, al
	inc dx
	shr eax, 8 
	loop .s

;清空高位
	and al, 0x0f
;设置属性位 1110
	or  al, 0xe0
	out dx, al

;第三步：向端口写入读命令
	mov dx, 0x1f7
	mov al, 0x20
	out dx, al

;第四步：等待硬盘准备
.not_ready:
;空操作，减少打扰硬盘
	nop
	in al, dx
;取第三位和第七位
	and al, 0x88

	cmp al, 0x08
	jnz .not_ready

	mov eax, edi
	mov edx, 256
	mul edx
	mov ecx, eax
	
	mov dx, 0x1f0

.go_on_read:
	in ax, dx
	mov [ebx], ax
	add ebx, 2
	loop .go_on_read
	ret

;==================================================================
kernel_init:
	xor eax, eax
	xor ebx, ebx
	xor ecx, ecx
	xor edx, edx
	
;获取每个Program header table 大小
	mov dx, [KERNEL_BIN_BASE_ADDR + 42]
;获取Program header table 偏移地址
	mov ebx, [KERNEL_BIN_BASE_ADDR + 28]
;得到基地址
	add ebx, KERNEL_BIN_BASE_ADDR
;获取Program header table 长度
	mov cx, [KERNEL_BIN_BASE_ADDR + 44]

;遍历Program header table

.each_segment:
	cmp byte [ebx], PT_NULL
	je .PTNULL
;段在文件中大小
	push dword [ebx + 16]
;段在文件中偏移地址
	mov eax, [ebx + 4]
	add eax, KERNEL_BIN_BASE_ADDR
;压入源地址
	push eax
;压入目的地地址
	push dword [ebx + 8]

	call mem_cpy
	add esp, 12
;空节, 进入下一条目
.PTNULL:
	add ebx, edx
	loop .each_segment
	ret

;复制内存三个参数 desc, src, count
;=====================================================================
mem_cpy:
	push ebp
	mov ebp, esp
;关键ecx入栈，外层循环用到
	push ecx

	cld
	mov edi, [ebp + 8]
	mov esi, [ebp + 12]
	mov ecx, [ebp + 16]
	rep movsb

	pop ecx
	pop ebp
	ret

	message db "hello loader"


