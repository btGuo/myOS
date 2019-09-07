#include <list.h>
#include <hash_table.h>
#include <dir.h>
#include <memory.h>
#include <pathparse.h>
#include <string.h>

#define MAX_NAME_LEN 64

/**
 * 目录项缓冲结构
 */
struct dirent_cache {
	struct hash_table ht;
	struct dirent_info *root;
};

/**
 * 用于哈希表查找
 * @note 目录项可能同名，需要比较名字和父亲指针
 */
struct dc_key {
	char *name;
	struct dirent_info *parent;
};

bool compare_dir_e(struct list_head *elem, void *key){
	struct dirent_info *m_dire = list_entry(struct dirent_info, hash_tag, elem);
	struct dc_key *dc_key = (struct dc_key *)key;

	if(strcmp(m_dire->filename, dc_key->name) == 0 &&
			m_dire->parent == dc_key->parent){

		return true;
	}
	return false;
}

void dc_init(struct dirent_cache *dc, struct dirent_info *root){

	dc->root = root;
	hash_table_init(&dc->ht, (hash_func)compare_dir_e, (hashf_func)hashf_str);
}

void dc_insert(struct dirent_cache *dc, 
		struct dirent_info *parent, 
		struct dirent_info *child){

	hash_table_insert(&dc->ht, &child->hash_tag, child->filename);
	child->parent = parent;
	//加入父亲链表中
	list_add(&child->parent_tag, &parent->child_list);
}
//TODO m_inode 不在内存中的情况

struct dirent_info *dc_search(struct dirent_cache *dc, 
		const char *path, char **pos){

	char name[MAX_NAME_LEN];
	struct dc_key key;
	key.name = name;
	struct dirent_info *walk = dc->root;
	struct list_head *lh = NULL;

	while((*pos = path_parse(path, name))){
		key.parent = walk;
		lh = hash_table_find(&dc->ht, &key);
		if(lh){
			walk = list_entry(struct dirent_info, hash_tag, lh);
		}else {
			break;
		}
	}

	return walk;
}

void _dc_remove(struct dirent_cache *dc,
		struct dirent_info *di){
	
	struct list_head *head = &di->child_list;
	struct list_head *walk = head->next;
	struct dirent_info *dir_child;

	while(walk != head){
		dir_child = list_entry(struct dirent_info, parent_tag, walk);
		list_del(&dir_child->hash_tag);
		//递归
		_dc_remove(dc, dir_child);
		walk = walk->next;
		
	}
	kfree(di);
}

/**
 * 删除缓存中该目录及其孩子
 */
void dc_remove(struct dirent_cache *dc, 
		const char *path){

	char *pos = NULL;
	struct dirent_info *di = dc_search(dc, path, &pos);
	if(pos){
		return;
	}
	
	//该目录项在缓存中，遍历所有的孩子，全部移除
	_dc_remove(dc, di);
}






