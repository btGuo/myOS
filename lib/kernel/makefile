BUILD_DIR = ../../build
LIB = -I ../../include
OUT = $(BUILD_DIR)/libk.a

CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall -D__LIB_KERNEL -fno-stack-protector -fno-pie
AR = ar
LD = ld
LDFLAGS = -m elf_i386 -r

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)

-include $(DEPS)

all: comp $(OUT)

$(OUT):$(OBJS)
	$(AR) rcs $(OUT) $(OBJS)

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

.PHONY: clean comp

clean:
	rm $(OBJS) $(DEPS)

comp:
	./comp.sh

