%include "boot.inc"
section loader vstart=LOADER_BASE_ADDR
LOADER_STACK_TOP equ LOADER_BASE_ADDR
jmp loader_start
gdt:
	dq 0
	dq 0x00_c_f_9_8_000000_ffff
	dq 0x00_c_f_9_2_000000_ffff
	dq 0x00_c_f_9_2_008000_0007

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

.mem_get_ok:
	mov [total_mem_bytes], edx 

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
	mov esp, LOADER_STACK_TOP
	mov ax, SELECTOR_VIDEO
	mov gs, ax
	
	call setup_page
	sgdt [gdt_ptr]
	mov ebx, [gdt_ptr + 2]
	or  dword [ebx + 0x18 + 4], 0xc0000000

	add dword [gdt_ptr + 2], 0xc0000000
	add esp, 0xc0000000

	mov eax, PAGE_DIR_TABLE_POS
	mov cr3, eax

	mov eax, cr0
	or  eax, 0x80000000
	mov cr0, eax

	lgdt [gdt_ptr]

	mov byte [gs:160], 'V'
	jmp $

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
;指向低端1M空间
	mov ecx, 0x100
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
	message db "hello loader"
