CC = gcc
LD = ld

CFLAGS = -I ../../include -c -fno-builtin -m32 -Wall -Wsystem-headers
LDFLAGS = -e main -m elf_i386 

OBJS = ./main.o ../../build/_lib_user.o ../../build/assert.o ../../build/print.o\

OUT = main.bin

all:$(OUT)

main.o : main.c
	$(CC) $(CFLAGS) $< -o $@

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

writehd:
	dd if=$(OUT) of=../../hd60.img bs=512 count=10 seek=300 conv=notrunc


