BUILD_DIR = ../../build
LIB = -I ../../include
OUT = $(BUILD_DIR)/_lib_kernel.o

CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall -D__LIB_KERNEL
LD = ld
LDFLAGS = -m elf_i386 -r

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)

-include $(DEPS)

all:$(OUT)

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

.PHONY: clean

clean:
	rm $(OBJS) $(DEPS)
