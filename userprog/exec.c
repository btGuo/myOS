#include <stdint.h>
#include <fs.h>
#include <string.h>
#include <memory.h>
#include <thread.h>
#include <fs_sys.h>
#include <vm_area.h>
#include <process.h>
#include <elf32.h>
#include <interrupt.h>
#include <debug.h>

static void print_Elf32_phdr(struct Elf32_Phdr *prog_h){
	printk("elf program header info :\n");
	printk("offset      %d\n", prog_h->p_offset);
	printk("vaddr       %h\n", prog_h->p_vaddr);
	printk("file size   %d\n", prog_h->p_filesz);
	printk("memory size %d\n", prog_h->p_memsz);
	printk("\n");
}


extern void intr_exit(void);

/**
 * 加载段到内存中
 * @param fd 文件描述符
 * @param prog_header 段头
 *
 * @return 是否成功
 */
static int32_t segmemt_load(int32_t fd, struct Elf32_Phdr *prog_header){
#ifdef DEBUG
	print_Elf32_phdr(prog_header);
#endif

	uint32_t vaddr = prog_header->p_vaddr & 0xfffff000;
	uint32_t off   = prog_header->p_vaddr & 0x00000fff;
	uint32_t size  = prog_header->p_filesz;
	uint32_t sz    = 0;

	struct vm_area *vm =  vm_area_alloc(prog_header->p_vaddr, size);
	if(!vm){
		printk("vm alloc failed\n");
		return false;
	}
	vm_area_add(vm);

	while(sz < size){

		uint32_t *pde = PDE_PTR(vaddr);
		uint32_t *pte = PTE_PTR(vaddr);
		if(!(*pde & 0x1) || !(*pte & 0x1)){
			if(get_a_page(PF_USER, vaddr) == NULL){
				return false;
			}
		}

		vaddr += PG_SIZE;
		if(!sz && off)
			sz += off;
		else sz += PG_SIZE;
	}
	sys_lseek(fd, prog_header->p_offset, SEEK_SET);
	sys_read(fd, (void *)prog_header->p_vaddr, size);
	return true;
}

/**
 * 加载程序到内存中
 *
 * @param path 程序路径
 * @return 在内存中的起始地址
 * 	@retval -1 加载失败
 */
static int32_t load(const char *path){
	
	printk("exec load start\n");
	const uint32_t EH_SIZE = sizeof(struct Elf32_Ehdr);
	const uint32_t PH_SIZE = sizeof(struct Elf32_Phdr);

	int32_t ret = -1;
	struct Elf32_Ehdr elf_header;
	struct Elf32_Phdr prog_header;
	memset(&elf_header, 0, EH_SIZE);

	int32_t fd = sys_open(path, O_RDONLY);
	if(fd == -1){
		printk("load open file failed\n");
		return -1;
	}

	if(sys_read(fd, &elf_header, EH_SIZE) != EH_SIZE){
		printk("read failed\n");
		ret = -1;
		goto done;
	}

	//验证elf头
	
	if(memcmp(elf_header.e_ident, "\x7f\x45\x4c\x46\x1\x1\x1", 7) || \
			elf_header.e_type    != 2 || \
			elf_header.e_machine != 3 || \
			elf_header.e_version != 1 || \
			elf_header.e_phnum > 1024 || \
			elf_header.e_phentsize != PH_SIZE){
		
		ret = -1;
		goto done;
	}
	
	Elf32_Off  ph_off  = elf_header.e_phoff;

	uint32_t i = 0;
	while(i < elf_header.e_phnum){
		memset(&prog_header, 0, PH_SIZE);

		//每次都重新定位
		sys_lseek(fd, ph_off, SEEK_SET);

		if(sys_read(fd, &prog_header, PH_SIZE) != \
				PH_SIZE){
			ret = -1;
			goto done;
		}

		//可加载
		if(prog_header.p_type == PT_LOAD){
			if(!segmemt_load(fd, &prog_header)){
				ret = -1;
				goto done;
			}
		}
		ph_off += PH_SIZE;
		++i;
	}
	ret = elf_header.e_entry;
done:
	sys_close(fd);
	printk("exec load done\n");
	return ret;
}
			
int32_t sys_execv(const char *path, const char **argv){

	printk("sys_execv start\n");
	uint32_t argc = 0;
	while(argv[argc])
		++argc;

	int32_t entry = load(path);
	if(entry == -1){
		printk("entry wrong\n");
		return -1;
	}
	printk("entry point ; %x\n", entry);

	strcpy(curr->name, path);

	struct intr_stack *stack0 = (struct intr_stack *)\
			((uint32_t)curr + PG_SIZE - sizeof(struct intr_stack));

	//命令行参数
	stack0->ebx = (int32_t)argv;
	stack0->ecx = argc;

	stack0->eip = (void *)entry;
	stack0->esp = (void *)0xc0000000;

	asm volatile("movl %0, %%esp; jmp intr_exit"::\
			"g"(stack0): "memory");
	return 0;
}
	
