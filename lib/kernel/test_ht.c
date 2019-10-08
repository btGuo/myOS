#ifdef __TEST
#include "hash_table.h"
#include "kernelio.h"
#include "string.h"
#include "list.h"

struct foo{
	struct list_head hash_tag;
	uint32_t key;
	char data;
};

#define KEY_LEN 32

struct strfoo{
	struct list_head hash_tag;
	char key[KEY_LEN];
	char data;
};

bool compare1(struct list_head *elem, void *key){
	struct foo *foo = list_entry(struct foo, hash_tag, elem);
	uint32_t *_key = (uint32_t *)key;
	return foo->key == *_key;
}

bool compare2(struct list_head *elem, void *key){
	struct strfoo *sf = list_entry(struct strfoo, hash_tag, elem);
	char *str = (char *)key;
	if(!strcmp(sf->key, str)){
		printk("hit\n");
		printk("key %s\n", sf->key);
		return true;
	}
	return false;
}

void test_int(){

	struct hash_table ht;
	hash_table_init(&ht, compare1, hashf_uint);
	struct foo foo;
	foo.key = 2354;
	foo.data = 'F';
	hash_table_insert(&ht, &foo.hash_tag, &foo.key);
	struct list_head *lh = hash_table_find(&ht, &foo.key);
	struct foo *f = list_entry(struct foo, hash_tag, lh);
	if(f->data != foo.data){
		printk("error");
	}
	printk("%c %d\n", f->data, f->key);
	hash_table_del(&ht);

}

void test_str(){
	
	struct hash_table ht;
	hash_table_init(&ht, compare2, hashf_str);
	struct strfoo foo;
	strcpy(foo.key, "i am a key");
	foo.data = 'F';
	hash_table_insert(&ht, &foo.hash_tag, foo.key);
	struct list_head *lh = hash_table_find(&ht, foo.key);
	if(!lh){
		printk("error lh == NULL\n");
		hash_table_del(&ht);
		return;
	}
	struct strfoo *f = list_entry(struct strfoo, hash_tag, lh);
	if(f->data != foo.data){
		hash_table_del(&ht);
		printk("error\n");
		return;
	}

	printk("%c %s\n", f->data, f->key);
	hash_table_del(&ht);
}
	
void test_hashtable(){
	test_str();
}
#endif
