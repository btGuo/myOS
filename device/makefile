BUILD_DIR = ../build
LIB = -I ../include
OUT = $(BUILD_DIR)/_device.o

CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 
LD = ld
LDFLAGS = -m elf_i386 -r

SRCS = $(wildcard *.c)
OBJS = $(SRCS:%.c=$(BUILD_DIR)/%.o)
DEPS = $(SRCS:%.c=$(BUILD_DIR)/%.d)

-include $(DEPS)

all:$(OUT)

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

$(OUT):$(OBJS)
	$(LD) $(LDFLAGS) $^ -o $@

.PHONY: clean 

clean:
	rm $(OBJS) $(DEPS) 

