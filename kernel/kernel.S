[bits 32]
%define ERROR_CODE nop

%define ZERO push 0

extern idt_table
;这里.data放前面，后面的会合并过来，形成表
section .data
global intr_entry_table
intr_entry_table:

%macro VECTOR 2
section .text
intr%1entry:
	
;中断例程
	%2
	push ds
	push es
	push fs
	push gs
	pushad
;向8259发送信号
	mov al, 0x20
	out 0xa0, al
	out 0x20, al

	mov eax, [esp + 4 * 12]
	push eax
	push %1
	call [idt_table + %1 * 4]
	jmp intr_exit

section .data
	dd intr%1entry

%endmacro

section .text
global intr_exit
intr_exit:
	add esp, 8 ;跳过pad 和vec_no两个参数
	popad
	pop gs
	pop fs
	pop es
	pop ds
	add esp, 4 ;跳过err_code
	iretd
;================================================================
;前32个中断向量，5个一组; intel 保留
VECTOR 0x00, ZERO
VECTOR 0x01, ZERO
VECTOR 0x02, ZERO
VECTOR 0x03, ZERO
VECTOR 0x04, ZERO
	

VECTOR 0x05, ZERO
VECTOR 0x06, ZERO
VECTOR 0x07, ZERO
VECTOR 0x08, ERROR_CODE
VECTOR 0x09, ZERO


VECTOR 0x0a, ERROR_CODE
VECTOR 0x0b, ERROR_CODE
VECTOR 0x0c, ERROR_CODE
VECTOR 0x0d, ERROR_CODE
VECTOR 0x0e, ERROR_CODE


VECTOR 0x0f, ZERO
VECTOR 0x10, ZERO
VECTOR 0x11, ERROR_CODE
VECTOR 0x12, ZERO
VECTOR 0x13, ZERO


VECTOR 0x14, ZERO
VECTOR 0x15, ZERO
VECTOR 0x16, ZERO
VECTOR 0x17, ZERO
VECTOR 0x18, ZERO


VECTOR 0x19, ZERO
VECTOR 0x1a, ZERO
VECTOR 0x1b, ZERO
VECTOR 0x1c, ZERO
VECTOR 0x1d, ZERO


VECTOR 0x1e, ZERO
VECTOR 0x1f, ZERO
;=================================================================
;以下中断号对应外中断

VECTOR 0x20, ZERO 			;时钟中断
VECTOR 0x21, ZERO 			;键盘中断
VECTOR 0x22, ZERO 			;级联用
VECTOR 0x23, ZERO 			;串口1
VECTOR 0x24, ZERO 			;串口2

VECTOR 0x25, ZERO 			;并口2 
VECTOR 0x26, ZERO 			;软盘 
VECTOR 0x27, ZERO                       ;并口1
VECTOR 0x28, ZERO                       ;实时时钟
VECTOR 0x29, ZERO  			;重定向IRQ2

VECTOR 0x2a, ZERO  			;保留
VECTOR 0x2b, ZERO 			;保留
VECTOR 0x2c, ZERO 			;ps/2鼠标
VECTOR 0x2d, ZERO 			;FPU异常
VECTOR 0x2e, ZERO 			;硬盘

VECTOR 0x2f, ZERO
VECTOR 0x30, ZERO

;=================================================================

[bits 32]
extern syscall_table
section .text
global syscall_handler
syscall_handler:
	
	push 0 ;没什么用的, 对齐中断栈中err_code
	push ds
	push es
	push fs
	push gs
	pushad

	push 0x80 ;同上
	;压入函数参数
	push edx
	push ecx
	push ebx

	call [syscall_table + eax * 4]
	;跳过参数
	add esp, 12
	push 0 ;对齐用的

	;写入返回值
	mov [esp + 9 * 4], eax
	jmp intr_exit

