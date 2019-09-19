LIB = -I include
BUILD_DIR = $(PWD)/build

CC = gcc
CFLAGS = $(LIB) -c -fno-builtin -m32 -Wall -fno-stack-protector
LD = ld
LDFALGS = -T $(PWD)/kernel/linker.ld -m elf_i386 

SUBDIRS = device kernel lib thread userprog fs test boot
OBJS = boot.o _kernel.o _thread.o _device.o _userprog.o _fs.o \
       _test.o  _lib_kernel.o

OUT = $(BUILD_DIR)/kernel.bin

final:
	cd $(BUILD_DIR) && $(LD) $(LDFALGS) -o $(OUT) $(OBJS)

all :mk_dir subdir final

.PHONY:mk_dir clean subdir objdump final

subdir:
	@for dir in $(SUBDIRS); do $(MAKE) -C $$dir all;done

mk_dir:
	if [ ! -d "$(BUILD_DIR)" ]; then mkdir $(BUILD_DIR); fi

clean:
	cd $(BUILD_DIR) && rm -rf ./*

objdump:
	objdump -d ./build/kernel.bin > ./build/kernel.uasm





