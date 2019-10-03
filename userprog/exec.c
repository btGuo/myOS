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
#include <kernelio.h>

#define DEBUG 1

#ifdef DEBUG

static void print_Elf32_phdr(struct Elf32_Phdr *prog_h){
	printk("elf program header info :\n");
	printk("offset      %d\n", prog_h->p_offset);
	printk("vaddr       %x\n", prog_h->p_vaddr);
	printk("file size   %d\n", prog_h->p_filesz);
	printk("memory size %d\n", prog_h->p_memsz);
	printk("\n");
}

#endif


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

#define MEM_CHECK \
if(bytes % PG_SIZE == 0){ \
	if(dest == svaddr){ \
		ret = -1; \
		goto error; \
	} \
	uint32_t paddr = palloc(PF_USER, 1); \
	uint32_t vaddr = (uint32_t)dest - PG_SIZE; \
	page_table_add(vaddr, paddr); \
	bytes = 0; \
}

#define dest_off(dest) \
((char *)0xc0000000 - (svaddr + MAX_ARG_PAGES * PG_SIZE - (dest)))

#define MAX_ARG_PAGES 64

static int32_t build_stack(
		struct intr_stack *stack0, 
		char *const argv[], 
		char *const envp[])
{
	printk("build_stack start\n");

	char *svaddr = (char *)vaddr_get(MAX_ARG_PAGES);
	if(svaddr == NULL){

		return -1;
	}

	int ret = 0;

	//统计数量
	uint32_t argc = 0;
	uint32_t envc = 0;
	while(argv[argc]) ++argc;
	while(envp[envc]) ++envc;

	//拉到最后，从后往前复制
	char *dest = svaddr + MAX_ARG_PAGES * PG_SIZE;

	//这里要加1，最后一个是NULL
	uint32_t _envpsz = sizeof(char *) * (envc + 1);
	uint32_t _argvsz = sizeof(char *) * (argc + 1);

	//新的指针数组
	char **_envp = kmalloc(_envpsz);
	char **_argv = kmalloc(_argvsz);

	//最后一项为空
	_envp[envc] = NULL;
	_argv[argc] = NULL;
	
	uint32_t bytes = 0;

	//复制两组字符串
	for(int i = 0; i < 2; i++){

		char *const* arr = NULL;
		char ** _arr = NULL;
		uint32_t cnt = 0;
		char * src = NULL;

		if(i == 0){
			cnt = envc; arr = envp; _arr = _envp;
		}else {
			cnt = argc; arr = argv; _arr = _argv;
		}
		
		while(cnt--){
	
			src = arr[cnt];
			uint32_t len = strlen(src) + 1;
			src += len;

			while(len--){

				MEM_CHECK
				//先减
				*--dest = *--src;
				++bytes;
			}
			_arr[cnt] = dest_off(dest);
		}
	}

	printk("bytes %d\n", bytes);

	uint32_t align = sizeof(char *) - 1;
	//对齐
	bytes += (uint32_t)dest & align;
	dest = (char *)((uint32_t)dest & ~align);
	
	printk("bytes %d\n", bytes);
	//记录指针数组位置
	char *_envpp = NULL;
	char *_argvp = NULL;

	//复制两组指针数组
	for(int i = 0; i < 2; i++){

		char *src = NULL;
		uint32_t len = 0;

		if(i == 0){
			src = (char *)_envp; len = _envpsz;
		}else {
			src = (char *)_argv; len = _argvsz;
		}

		src += len;
		while(len--){
			
			MEM_CHECK
			*--dest = *--src;
			++bytes;
		}
		if(i == 0){
			_envpp = dest_off(dest);
		}else{
			_argvp = dest_off(dest);
		}
	}

	//复制最后三个参数
	for(int i = 0; i < 3; i++){

		char *src = NULL;
		uint32_t len = 0;

		if(i == 0){
			src = (char *)&_envpp; len = sizeof(_envpp);
		}else if(i == 1){
			src = (char *)&_argvp; len = sizeof(_argvp);
		}else {
			src = (char *)&argc;   len = sizeof(argc);
		}

		src += len;
		while(len--){

			MEM_CHECK
			*--dest = *--src;
			++bytes;
		}
	}
	printk("bytes %d\n", bytes);

	//释放原来栈页
	uint32_t vaddr = 0xbffff000;

	while(pg_try_free(vaddr)) vaddr -= PG_SIZE;

	//直接更改映射
	uint32_t top = (uint32_t)svaddr + MAX_ARG_PAGES * PG_SIZE;
	uint32_t bottom = (uint32_t)dest & 0xfffff000;
	uint32_t paddr = 0;
	vaddr = 0xc0000000;

	printk("top %x\n", top);
	printk("bottom %x\n", bottom);
	while(top != bottom){

		top -= PG_SIZE;
		vaddr -= PG_SIZE;

		//拿到物理地址，重新映射
		paddr = addr_v2p(top);
		//删除旧项
		page_table_pte_remove(top);
		//重新映射
		page_table_add(vaddr, paddr);
	}

	printk("bytes %d\n", bytes);

	stack0->esp = (char *)0xc0000000 - (svaddr + MAX_ARG_PAGES * PG_SIZE - dest);
	printk("esp %x\n", (uint32_t)stack0->esp);
error:
	kfree(_argv);
	kfree(_envp);

	printk("build stack done\n");
	return ret;
}	
	
int sys_execve(const char *path, char *const argv[], char *const envp[]){

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

	if(build_stack(stack0, argv, envp) == -1){

		printk("build stack wrong\n");
		return -1;
	}

	//命令行参数
	//stack0->ebx = (int32_t)argv;
	//stack0->ecx = argc;

	stack0->eip = (void *)entry;
	//stack0->esp = (void *)0xc0000000;

	asm volatile("movl %0, %%esp; jmp intr_exit"::\
			"g"(stack0): "memory");
	return 0;
}


	
