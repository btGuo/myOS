BUILD_DIR = ./build
ENTRY_POINT = 0xc0001500
AS = nasm
CC = gcc
LD = ld
LIB = -I lib/ -I lib/kernel -I kernel -I device
ASFLAGS = -f elf
CFLAGS = $(LIB) -c -fno-builtin -m32 
LDFALGS = -Ttext $(ENTRY_POINT) -e main -Map $(BUILD_DIR)/kernel.map \
		  -m elf_i386
OBJS = $(BUILD_DIR)/main.o $(BUILD_DIR)/init.o $(BUILD_DIR)/interrupt.o \
	   $(BUILD_DIR)/timer.o $(BUILD_DIR)/kernel.o $(BUILD_DIR)/print.o \
	   $(BUILD_DIR)/debug.o $(BUILD_DIR)/string.o $(BUILD_DIR)/memory.o \
	   $(BUILD_DIR)/bitmap.o

################  c  ##################
$(BUILD_DIR)/main.o: kernel/main.c lib/kernel/print.h lib/stdint.h \
	kernel/init.h
	$(CC) $(CFLAGS) $<  -o $@

$(BUILD_DIR)/init.o: kernel/init.c lib/kernel/print.h kernel/init.h \
	lib/stdint.h kernel/interrupt.h device/timer.h
	$(CC) $(CFLAGS) $<  -o $@

$(BUILD_DIR)/interrupt.o: kernel/interrupt.c lib/kernel/print.h \
	kernel/interrupt.h lib/stdint.h kernel/global.h lib/kernel/io.h 
	$(CC) $(CFLAGS) $<  -o $@

$(BUILD_DIR)/timer.o: device/timer.c device/timer.h lib/kernel/print.h \
	lib/stdint.h lib/kernel/io.h
	$(CC) $(CFLAGS) $<  -o $@

$(BUILD_DIR)/debug.o: kernel/debug.c kernel/debug.h lib/stdint.h \
	kernel/interrupt.h lib/kernel/print.h
	$(CC) $(CFLAGS) $<  -o $@

$(BUILD_DIR)/string.o: lib/string.c kernel/debug.h lib/string.h
	$(CC) $(CFLAGS) $<  -o $@

$(BUILD_DIR)/bitmap.o: lib/kernel/bitmap.c lib/kernel/bitmap.h \
	lib/string.h kernel/global.h kernel/debug.h
	$(CC) $(CFLAGS) $<  -o $@

$(BUILD_DIR)/memory.o: kernel/memory.c kernel/memory.h lib/kernel/print.h \
	lib/kernel/bitmap.h lib/stdint.h
	$(CC) $(CFLAGS) $<  -o $@
	
############### asm ####################
$(BUILD_DIR)/kernel.o: kernel/kernel.S
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/print.o: lib/kernel/print.S
	$(AS) $(ASFLAGS) $< -o $@

################ link #####################
$(BUILD_DIR)/kernel.bin: $(OBJS)
	$(LD) $(LDFALGS) $^ -o $@

.PHONY : mk_dir hd clean all

mkdir:
	if[[ ! -d $(BUILD_DIR) ]];then mkdir $(BUILD_DIR);fi
hd:
	dd if=build/kernel.bin of=hd.img seek=9 count=200 bs=512 \
		conv=notrunc
clean:
	cd $(BUILD_DIR) && rm -rf ./*

build: $(BUILD_DIR)/kernel.bin 

all : mk_dir build hd





