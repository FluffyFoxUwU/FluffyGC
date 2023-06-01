#ifndef _headers_1682308214_FluffyGC_list_head
#define _headers_1682308214_FluffyGC_list_head

#include "bug.h"
#include "util.h"

struct list_head {
  struct list_head* next;
  struct list_head* prev;
};

static inline void list_head_init(struct list_head* list) {
  list->next = list;
  list->prev = list;
}

#define LIST_HEAD_INIT(x) {.next = &x, .prev = &x}
#define list_is_valid(head) ((head)->next != ((void*) 0xDEADBEE1) && (head)->next != NULL)

static inline void __list_add(struct list_head *new, struct list_head *prev, struct list_head *next) {
  BUG_ON(!list_is_valid(prev));
  BUG_ON(!list_is_valid(next));
  BUG_ON(list_is_valid(new));
  next->prev = new;
  new->next = next;
  new->prev = prev;
  prev->next = new;
}

static inline void __list_del(struct list_head* prev, struct list_head* next) {
  BUG_ON(!list_is_valid(prev));
  BUG_ON(!list_is_valid(next));
  next->prev = prev;
  prev->next = next;
}

static inline void list_add(struct list_head* new, struct list_head* head) {
  BUG_ON(list_is_valid(new));
  BUG_ON(!list_is_valid(head));
	__list_add(new, head, head->next);
}

static inline void list_add_tail(struct list_head* new, struct list_head* head) {
  BUG_ON(list_is_valid(new));
  BUG_ON(!list_is_valid(head));
	__list_add(new, head->prev, head);
}

static inline void list_init_as_invalid(struct list_head* entry) {
  entry->next = (void*) 0xDEADBEE1;
	entry->prev = (void*) 0xDEADBEE2;
}

static inline void list_del(struct list_head* entry) {
  BUG_ON(!list_is_valid(entry));
  
	__list_del(entry->prev, entry->next);
  list_init_as_invalid(entry);
}

#define list_is_empty(head) ((head)->next == (head))
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

