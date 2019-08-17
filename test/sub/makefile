CC = gcc
LD = ld

CFLAGS = -I ../../include -c -fno-builtin -m32 -Wall -Wsystem-headers
LDFLAGS = -m elf_i386 

OBJS = ./main.o ../../lib/user/build/lib_user.o

OUT = main.bin 

all:$(OUT) writehd dump 

main.o : main.c
	$(CC) $(CFLAGS) $< -o $@

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

writehd:
	dd if=$(OUT) of=../../hd60.img bs=512 count=100 seek=300 conv=notrunc

dump:
	objdump -d $(OUT) >tmp

