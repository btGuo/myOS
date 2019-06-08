#ifndef __KERNEL_LIST_H
#define __KERNEL_LIST_H
#include"global.h"
#include"print.h"
#include"stdint.h"

#define offsetof(type, member) ((int)(&((type*)0)->member))
#define list_entry(type, member, ptr)({ \
	const typeof(((type*)0)->member) *__mptr = (ptr); \
	(type *)((char *)__mptr - offsetof(type, member));})

struct list_head{
	struct list_head *prev;
	struct list_head *next;
};

//typedef bool function(struct list_head*, int arg);

#define __LIST_HEAD_INIT(name){ &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = __LIST_HEAD_INIT(name)

#define LIST_HEAD_INIT(name)\
do{ (name).next=&(name);    \
	(name).prev=&(name);   \
}while(0)


static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(struct list_head *new, struct list_head *head)
{
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
	__list_add(new, head->prev, head);
}

static inline void list_del(struct list_head *entry)
{
	entry->prev->next = entry->next;
	entry->next->prev = entry->prev;
}

static inline bool list_empty(struct list_head *head)
{
	return head->next == head;
}

static inline uint32_t list_len(struct list_head *head)
{
	struct list_head *tmp = head->next;
	uint32_t len = 0;
	while(tmp != head)
	{
		++len;
		tmp = tmp->next;
	}
	return len;
}

static inline bool elem_find(struct list_head *head, struct list_head *elem)
{
	struct list_head *tmp = head->next;
	while(tmp != head)
	{
		if(tmp == elem)
			return true;
		tmp = tmp->next;
	}
	return false;
}

static inline void for_each(struct list_head *head)
{
	struct list_head *tmp = head->next;
	put_str("print list\n");
	while(tmp != head)
	{
		put_int((uint32_t)tmp);put_char('\n');
		tmp = tmp->next;
	}
	put_str("end print list\n");
}

#endif
