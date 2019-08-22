BUILD_DIR = ../build
LIB = -I ../include
OUT = $(BUILD_DIR)/_kernel.o

AS = nasm
ASFLAGS = -f elf
CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall -fno-stack-protector
LD = ld
LDFLAGS = -m elf_i386 -r

_SRCS = $(wildcard *.c)
SRCS = $(_SRCS:main.c= )
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)

OBJS +=$(BUILD_DIR)/kernel.o

-include $(DEPS)

all:$(OUT)

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/kernel.o:kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

.PHONY: clean

clean:
	rm $(OBJS) $(DEPS) 
