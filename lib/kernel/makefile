BUILD_DIR = ../../build
LIB = -I ../../include
OUT = $(BUILD_DIR)/_lib_kernel.o

CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall -D__LIB_KERNEL -fno-stack-protector
LD = ld
LDFLAGS = -m elf_i386 -r

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)

-include $(DEPS)

all: copy $(OUT)

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

.PHONY: clean copy

clean:
	rm $(OBJS) $(DEPS)

copy:
	cp ../user/syscall.c ../user/stdio.c ../user/string.c .
