#ifndef __LIBARCH_INTEL_BOOT_H
#define __LIBARCH_INTEL_BOOT_H

/** The magic number for the Multiboot header. */
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

/** The flags for the Multiboot header. */
#define MULTIBOOT_HEADER_FLAGS          0x00000003

/** Size of the multiboot header structure. */
#define MULTIBOOT_HEADER_SIZE           52

/** The magic number passed by a Multiboot-compliant boot loader.  */
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

#include <stdint.h>

/**
 * The symbol table for a.out.
 */
struct AoutSymbolTable
{
    uint32_t tabSize;
    uint32_t strSize;
    uint32_t address;
    uint32_t reserved;
};
        
/**
 * The section header table for ELF.
 */
struct ElfSectionHeaderTable
{
    uint32_t num;
    uint32_t size;
    uint32_t address;
    uint32_t shndx;
};

/**
 * The Multiboot information.
 */
struct multiboot_info
{
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_address;

    union
    {
        struct AoutSymbolTable aout;
        struct ElfSectionHeaderTable elf;
    };
    uint32_t mmap_length;
    uint32_t mmap_address;
    uint32_t drivers_length;
    uint32_t drives_addr;
};

/**
 * The module class.
 */
struct MultibootModule
{
    uint32_t modStart;
    uint32_t modEnd;
    uint32_t string;
    uint32_t reserved;
};

/**
 * The memory map. Be careful that the offset 0 is base_addr_low
 * but no size.
 */
struct MultibootMemoryMap
{
    uint32_t size;
    uint32_t baseAddressLow;
    uint32_t baseAddressHigh;
    uint32_t lengthLow;
    uint32_t lengthHigh;
    uint32_t type;
};

struct drive_info
{
	uint32_t size;
	uint8_t drive_number;
	uint8_t drive_mode;
	uint16_t drive_cylinders;
	uint8_t drive_heads;
	uint8_t drive_sectors;
	uint16_t drive_ports[0];
};

void set_info(struct multiboot_info *_info);
uint32_t get_mem_upper();
uint32_t get_mem_lower();
void print_drive_info();

#endif /* __LIBARCH_INTEL_BOOT_H */
