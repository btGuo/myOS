[bits 32]
section .text
global switch_to
switch_to:
;保存当前环境
	push esi
	push edi
	push ebx
	push ebp
	mov eax, [esp + 20]
	mov [eax], esp
;准备下个环境
	mov eax, [esp + 24]
	mov esp, eax
	pop ebp
	pop ebx
	pop edi
	pop esi
	ret
