CC = gcc
LD = ld

CFLAGS = -I ../include -c -fno-builtin -m32 -Wall -Wsystem-headers
LDFLAGS = -e main

OBJS = ../build/string.o ../build/syscall.o ../build/stdio.o ../build/debug.o\
       ./test_exec.o ../build/vsprintf.o

OUT = test_exec

all:$(OUT)

test_exec.o : test_exec.c
	$(CC) $(CFLAGS) $< -o $@

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@




