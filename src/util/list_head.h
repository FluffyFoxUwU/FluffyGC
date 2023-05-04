#ifndef _headers_1682308214_FluffyGC_list_head
#define _headers_1682308214_FluffyGC_list_head

#include "util.h"

struct list_head {
  struct list_head* next;
  struct list_head* prev;
};

static inline void list_head_init(struct list_head* list) {
  list->next = list;
  list->prev = list;
}

static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

static inline void __list_del(struct list_head* prev, struct list_head* next) {
  next->prev = prev;
  prev->next = next;
}

static inline void list_add(struct list_head* new, struct list_head* head) {
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head* new, struct list_head* head) {
	__list_add(new, head->prev, head);
}

static inline void list_del(struct list_head* entry) {
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

#define list_is_head(list, head) (list == head)
#define list_for_each(pos, head) for (pos = (head)->next; !list_is_head(pos, (head)); pos = pos->next)
#define list_entry(ptr, type, member) container_of(ptr, type, member)
#define list_first_entry(ptr, type, member) list_entry((ptr)->next, type, member)
#define list_last_entry(ptr, type, member) list_entry((ptr)->prev, type, member)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; \
	     !list_is_head(pos, (head)); \
	     pos = n, n = pos->next)

#endif

