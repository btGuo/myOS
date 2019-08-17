[bits 32]
extern main
extern exit
section .text
global _start
_start:
	push ebx ;argc
	push ecx ;argv
	call main

	push eax
	call exit

