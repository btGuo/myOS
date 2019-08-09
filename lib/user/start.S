[bits 32]
extern main
section .text
global _start
_start:
	push ebx ;argc
	push ecx ;argv
	call main

	push eax
	call exit

