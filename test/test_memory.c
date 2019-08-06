#include "memory.h"
#include "print.h"

void test_k_pages(){

	uint32_t *p, *p2;

	p = get_kernel_pages(1);
	printk("vaddr : %h\n", (uint32_t)p);
	p2 = get_kernel_pages(1);
	printk("vaddr : %h\n", (uint32_t)p2);
	free_kernel_pages(p);
	free_kernel_pages(p2);

	p = get_kernel_pages(10);
	printk("vaddr : %h\n", (uint32_t)p);
	free_kernel_pages(p);
	
	p = get_kernel_pages(20);
	printk("vaddr : %h\n", (uint32_t)p);

	p2 = get_kernel_pages(30);
	printk("vaddr : %h\n", (uint32_t)p2);
	
	free_kernel_pages(p);
	free_kernel_pages(p2);
}

void test_v_pages(){

	uint32_t *p1, *p2;
	p1 = vmalloc(1);
	vfree(p1);

	p1 = vmalloc(10);
	p2 = vmalloc(2);
	uint32_t cnt = 100;
	while(cnt--)
		*p1 = 10;

	vfree(p1);
	vfree(p2);
	p1 = vmalloc(23);
	p2 = vmalloc(11);
	vfree(p1);
	vfree(p2);
	p2 = vmalloc(21);
}

void test_memory(){
	test_v_pages();
}
