SELECTOR_VIDEO equ 0x0018

[bits 32]
section .text
;=======================================================
;打印字符串
global put_str
put_str:
	push ebx
	push edx
	xor edx, edx
	mov ebx, [esp + 12]
.goon:
	mov dl, [ebx]
	cmp dl, 0
	jz .str_over
	push edx
	call put_char
	add esp, 4
	inc ebx
	jmp .goon
.str_over:
	pop edx
	pop ebx
	ret

global put_char
;注意标号位置
;========================================================
;打印字符, 输入字符
put_char:
	pushad
	mov ax, SELECTOR_VIDEO
	mov gs, ax

;获得当前光标位置
;获取高8位
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	in al, dx
	mov ah, al
;获取低8位
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	in al, dx

	mov bx, ax
	mov ecx, [esp + 36]

	cmp cl, 0xd
	jz .is_carriage_return
	cmp cl, 0xa
	jz .is_line_feed
	cmp cl, 0x8
	jz .is_backspace 
	jmp .put_other

;处理退格符
.is_backspace:
	dec bx
	shl bx, 1

	mov byte [gs:bx], 0x20
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	jmp .set_cursor

;处理普通字符
.put_other:
	shl bx, 1
	mov [gs:bx], cl
	inc bx
	mov byte [gs:bx], 0x07
	shr bx, 1
	inc bx
	cmp bx, 2000
	jl .set_cursor

;处理回车和换行
.is_line_feed:
.is_carriage_return:
	xor dx, dx
	mov ax, bx
	mov si, 80
	div si
	sub bx, dx
.is_carriage_return_end:
	add bx, 80
	cmp bx, 2000
.is_line_feed_end:
	jl .set_cursor

;滚屏操作, 直接覆盖的方式
;每次4字节, 替换前面行
	cld
	mov ecx, 960
	mov esi, 0xc00b80a0
	mov edi, 0xc00b8000
	rep movsd

	mov ebx, 3840
	mov ecx, 80
;清空最后一行
.cls:
	mov word[gs:ebx], 0x0720
	add ebx, 2
	loop .cls
;重新设置光标
	mov bx, 1920

.set_cursor:
;设置高8位
	mov dx, 0x03d4
	mov al, 0x0e
	out dx, al
	mov dx, 0x03d5
	mov al, bh
	out dx, al
;设置低8位
	mov dx, 0x03d4
	mov al, 0x0f
	out dx, al
	mov dx, 0x03d5
	mov al, bl
	out dx, al
.put_char_done:
	popad
	ret
