#include "memory.h"
#include "debug.h"

void test_k_pages(){

	uint32_t *p, *p2;

	p = get_kernel_pages(1);
	printk("vaddr : %x\n", (uint32_t)p);

	for(int i = 0; i < 1024; i++)
	{
		p[i] = i;
	}
	/*
	p2 = get_kernel_pages(1);
	printk("vaddr : %x\n", (uint32_t)p2);
	free_kernel_pages(p);
	free_kernel_pages(p2);

	p = get_kernel_pages(10);
	printk("vaddr : %x\n", (uint32_t)p);
	free_kernel_pages(p);
	
	p = get_kernel_pages(20);
	printk("vaddr : %x\n", (uint32_t)p);

	p2 = get_kernel_pages(30);
	printk("vaddr : %x\n", (uint32_t)p2);
	
	free_kernel_pages(p);
	free_kernel_pages(p2);
	*/
	printk("test get_kernel_pages done, all right\n");
}

void test_v_pages(){

	uint32_t *p1, *p2;
	p1 = vmalloc(1);
	printk("vmalloc done\n");
	vfree(p1);
	printk("v1 done\n");

	p1 = vmalloc(10);
	p2 = vmalloc(2);
	printk("p1, p2\n");
	uint32_t cnt = 100;
	while(cnt--)
		*p1 = 10;

	vfree(p1);
	vfree(p2);

	printk("v2 done\n");

	p1 = vmalloc(23);
	p2 = vmalloc(11);
	vfree(p1);
	vfree(p2);
	printk("v3 done\n");
	p2 = vmalloc(21);
	vfree(p2);

	printk("test vmalloc, vfree done, all right\n");
}

void test_kmalloc(){
	uint32_t *p1, *p2;
	int size = 6433;
	p1 = kmalloc(size);
	int i = size;
	while(i--){
		p1[i] = 248;
	}
	kfree(p1);
	printk("test kmalloc, kfree done, all right\n");
}
	
void test_memory(){
	test_k_pages();
//	test_v_pages();
//	test_kmalloc();
}
