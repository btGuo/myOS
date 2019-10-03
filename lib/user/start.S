[bits 32]
extern main
extern exit
section .text
global _start
_start:
	call main

	push eax
	call exit

