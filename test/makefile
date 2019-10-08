LIB = -I ../include
ROOTFS = ../mkfext/root_fs/bin

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
OUTS = $(SRCS:.c=.bin)

CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall -fno-stack-protector -fno-pie -DMYOS
LD = ld
LDFLAGS = -m elf_i386

.PHONY: all install clean

all: $(OUTS)

%.o:%.c
	$(CC) $(CFLAGS) $< -o $@

%.bin:%.o
	$(LD) $(LDFLAGS) $^ -o $@ -L ../build -lc

install:
	@for file in $(OUTS); do cp $$file $(ROOTFS)/$${file%.*};done

clean:
	rm $(OUTS) $(OBJS)
