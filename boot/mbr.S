;主引导程序
;-----------------------------------
%include "boot.inc"
section mbr vstart=0x7c00
	mov ax, cs
	mov ds, ax
	mov es, ax
	mov ss, ax
	mov fs, ax
	mov sp, 0x7c00
;利用0x10号中断的0x06号功能清屏
	mov ax, 0x600
	mov bx, 0x700
	mov cx, 0
	mov dx, 0x184f

	int 0x10
;cx 字符串长度
;dl 指定颜色	
	mov cx, 9
	mov si, message
	mov ax, 0xb800
	mov es, ax
	mov di, 0	
	mov dl, 0xa4
;向显存写入字符	
s:  mov dh, [si]
	inc si
	mov [es: di], dh
	inc di
	mov [es: di], dl
	inc di
	loop s

	mov eax, LOADER_START_SECTOR
	mov bx, LOADER_BASE_ADDR
	mov cx, 4
	call rd_disk_16

	jmp LOADER_BASE_ADDR
;读取硬盘
;eax 起始lba地址
;bx  写入地址
;cx  读取扇区数
rd_disk_16:
	mov esi, eax
	mov di,  cx
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

	mov ax, di
	mov dx, 256
	mul dx
	mov cx, ax
	
	mov dx, 0x1f0

.go_on_read:
	in ax, dx
	mov [bx], ax
	add bx, 2
	loop .go_on_read
	ret

	message db "hello MBR"
	times 510 - ($ - $$) db 0
	db 0x55, 0xaa
