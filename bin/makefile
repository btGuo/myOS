LIB = -I ../include
ROOTFS = ../mkfext/root_fs/bin

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
OUTS = $(SRCS:.c=)

CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall -fno-stack-protector -fno-pie -DMYOS
LD = ld
LDFLAGS = -m elf_i386

.PHONY: all install clean

all:$(OBJS) $(OUTS)

%.o:%.c
	$(CC) $(CFLAGS) $< -o $@

%:%.o
	$(LD) $(LDFLAGS) $^ -o $@ -L ../build -lc

install:
	cp $(OUTS) $(ROOTFS) 

clean:
	rm $(OUTS) $(OBJS)
