ENTRY_POINT = 0xc0001500
LIB = -I include
BUILD_DIR = $(PWD)/build

AS = nasm
ASFLAGS = -f elf
CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall
LD = ld
LDFALGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map \
		  -m elf_i386

SUBDIRS = device kernel lib thread lib/kernel userprog
OBJS =$(BUILD_DIR)/main.o $(BUILD_DIR)/_kernel.o $(BUILD_DIR)/_thread.o $(BUILD_DIR)/_lib.o  \
	   $(BUILD_DIR)/_device.o $(BUILD_DIR)/_lib_kernel.o $(BUILD_DIR)/_userprog.o
OUT = $(BUILD_DIR)/kernel.bin

-include $(BUILD_DIR)/main.d

$(BUILD_DIR)/main.o:./kernel/main.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

$(OUT):$(OBJS)
	$(LD) $(LDFALGS) -o $(OUT) $(OBJS)

hd.img:$(OUT)
	dd if=$(OUT) of=hd.img seek=9 count=200 bs=512 \
		conv=notrunc

all :mk_dir subdir $(OUT) hd.img objdump

.PHONY:mk_dir clean subdir objdump

subdir:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir all;done

mk_dir:
	if [ ! -d "$(BUILD_DIR)" ]; then mkdir $(BUILD_DIR); fi

clean:
	cd $(BUILD_DIR) && rm -rf ./*

objdump:
	objdump -D ./build/kernel.bin > ./build/kernel.uasm

