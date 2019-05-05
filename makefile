ENTRY_POINT = 0xc0001500
LIB = -I include
BUILD_DIR = $(PWD)/build

AS = nasm
ASFLAGS = -f elf
CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 
LD = ld
LDFALGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map \
		  -m elf_i386

SUBDIRS = device kernel lib thread lib/kernel
OBJS =$(BUILD_DIR)/main.o $(BUILD_DIR)/_kernel.o $(BUILD_DIR)/_thread.o $(BUILD_DIR)/_lib.o  \
	   $(BUILD_DIR)/_device.o $(BUILD_DIR)/_lib_kernel.o

-include $(BUILD_DIR)/main.d

$(BUILD_DIR)/main.o:./kernel/main.c
	$(CC) $(CFLAGS) $< -o $@ -MMD -MP

subdir:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir;done

.PHONY:all mk_dir build hd clean subdir

hd:
	dd if=build/kernel.bin of=hd.img seek=9 count=200 bs=512 \
		conv=notrunc

mk_dir:
		if[[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi

build:$(OBJS)
	$(LD) $(LDFALGS) -o $(BUILD_DIR)/kernel.bin $(OBJS)

clean:
	cd $(BUILD_DIR) && rm -rf ./*

all :subdir build hd 


