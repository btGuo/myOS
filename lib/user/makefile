BUILD_DIR = ./build
LIB = -I ../../include
OUT = $(BUILD_DIR)/libc.a

AS = nasm
ASFLAGS = -f elf
CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32  -Wall -D__LIB_USER -fno-stack-protector -fno-pie
AR = ar

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)

OBJS += $(BUILD_DIR)/start.o

-include $(DEPS)

all : mk_dir $(OUT)

$(OUT):$(OBJS)
	$(AR) rcs $(OUT) $(OBJS)

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

$(BUILD_DIR)/start.o:start.S
	$(AS) $(ASFLAGS) $< -o $@


.PHONY: clean mk_dir 

clean:
	rm $(OBJS) $(DEPS) 

mk_dir:
	if [ ! -d "$(BUILD_DIR)" ]; then mkdir $(BUILD_DIR); fi

