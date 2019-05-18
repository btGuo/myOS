AS = nasm

all: loader mbr hd

loader:loader.S
	$(AS) $<

mbr:mbr.S
	$(AS) $<

hd:
	dd if=mbr of=../hd.img count=1 bs=512 conv=notrunc ;\
		dd if=loader of=../hd.img count=4 bs=512 seek=2 conv=notrunc
