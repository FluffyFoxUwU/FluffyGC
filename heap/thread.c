#include <stdlib.h>

#include <flup/data_structs/list_head.h>

#include "gc/gc.h"
#include "heap/heap.h"
#include "memory/arena.h"
#include "thread.h"

struct thread* thread_new(struct heap* owner) {
  struct thread* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct thread) {
    .ownerHeap = owner,
    .rootEntries = FLUP_LIST_HEAD_INIT(self->rootEntries),
    .rootSize = 0
  };
  return self;
}

void thread_free(struct thread* self) {
  flup_list_head* current;
  flup_list_head* next;
  flup_list_for_each_safe(&self->rootEntries, current, next)
    thread_unref_root_no_gc_block(self, flup_list_entry(current, struct root_ref, node));
  free(self);
}

struct root_ref* thread_new_root_ref_no_gc_block(struct thread* self, struct arena_block* block) {
  struct root_ref* ref = thread_prealloc_root_ref(self);
  if (!ref)
    return NULL;
  
  thread_new_root_ref_from_prealloc_no_gc_block(self, ref, block);
  return ref;
}

void thread_unref_root_no_gc_block(struct thread* self, struct root_ref* ref) {
  self->rootSize--;
  flup_list_del(&ref->node);
  
  gc_need_remark(ref->obj);
  free(ref);
}

// Preallocation
struct root_ref* thread_prealloc_root_ref(struct thread* self) {
  (void) self;
  return malloc(sizeof(struct root_ref));
}

void thread_new_root_ref_from_prealloc_no_gc_block(struct thread* self, struct root_ref* prealloc, struct arena_block* block) {
  prealloc->obj = block;
  self->rootSize++;
  flup_list_add_head(&self->rootEntries, &prealloc->node);
}


