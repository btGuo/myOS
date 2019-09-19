BUILD_DIR = ./build
LIB = -I ../../include
OUT = $(BUILD_DIR)/lib_user.o

AS = nasm
ASFLAGS = -f elf
CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32  -Wall -D__LIB_USER -fno-stack-protector -fno-pie
LD = ld
LDFLAGS = -m elf_i386 -r

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)

OBJS += $(BUILD_DIR)/start.o

-include $(DEPS)

all:$(OUT)

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

$(BUILD_DIR)/start.o:start.S
	$(AS) $(ASFLAGS) $< -o $@


.PHONY: clean 

clean:
	rm $(OBJS) $(DEPS) 

