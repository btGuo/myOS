/* The bootloader will look at this image and start execution at the symbol
   designated at the entry point. */
ENTRY(_start)
/* Tell where the various sections of the object files will be put in the final
   kernel image. */
phys = 1M + 8K;

SECTIONS
{
	/** boot 阶段，加载地址同运行地址相同 */
	. = 1M;
	.header : AT(1M)
	{
		/* multiboot 必须放在最前面 */
		boot.o(.multiboot)
		boot.o(.text)
		boot.o(.data)
		boot.o(.bss)
	}
	
	/** 内核定位于高端地址，运行地址比加载地址多了0xc0000000的偏移 */
	. = 0xc0102000;
	
	.text  : AT(phys)
	{
		text = .;
		
		^_.*(.text)
	}

	/* Read-only data. */
	.rodata : AT(phys + (rdata - text))
	{
		rdata = .;
		^_.*(.rodata)
	}

	/* Read-write data (initialized) */
	.data : AT(phys + (data - text))
	{
		data = .;
		^_.*(.data)
	}

	/* Read-write data (uninitialized) and stack */
	.bss : AT(phys + (bss - text))
	{
		bss = .;
		^_.*(COMMON)
		^_.*(.bss)
	}

	/* The compiler may produce other sections, put them in the proper place in
	   in this file, if you'd like to include them in the final kernel. */
}
