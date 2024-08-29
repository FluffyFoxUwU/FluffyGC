#include <stdlib.h>

#include <flup/data_structs/list_head.h>

#include "gc/gc.h"
#include "gc/gc_lock.h"
#include "heap/heap.h"
#include "memory/alloc_tracker.h"
#include "thread.h"

struct thread* thread_new(struct heap* owner) {
  struct thread* self = malloc(sizeof(*self) + sizeof(void*) * THREAD_LOCAL_REMARK_BUFFER_SIZE);
  if (!self)
    return NULL;
  
  *self = (struct thread) {
    .ownerHeap = owner,
    .rootEntries = FLUP_LIST_HEAD_INIT(self->rootEntries),
    .cachedRootEntries = FLUP_LIST_HEAD_INIT(self->cachedRootEntries),
    .rootSize = 0,
    .localRemarkBufferUsage = 0
  };
  
  if (!(self->allocContext = alloc_tracker_new_context(self->ownerHeap->gen->allocTracker)))
    goto failure;
  if (!(self->gcLockPerThread = gc_lock_new_thread(owner->gen->gcState->gcLock)))
    goto failure;
  return self;

failure:
  thread_free(self);
  return NULL;
}

void thread_free(struct thread* self) {
  if (!self)
    return;
  
  gc_lock_free_thread(self->ownerHeap->gen->gcState->gcLock, self->gcLockPerThread);
  flup_list_head* current;
  flup_list_head* next;
  flup_list_for_each_safe(&self->rootEntries, current, next)
    thread_unref_root_no_gc_block(self, flup_list_entry(current, struct root_ref, node));
  alloc_tracker_free_context(self->ownerHeap->gen->allocTracker, self->allocContext);
  free(self);
}

struct root_ref* thread_new_root_ref_no_gc_block(struct thread* self, struct alloc_unit* block) {
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
  
  // Add to cache
  flup_list_add_head(&self->cachedRootEntries, &ref->node);
}

// Preallocation
struct root_ref* thread_prealloc_root_ref(struct thread* self) {
  // Check if cache has it
  if (!flup_list_is_empty(&self->cachedRootEntries)) {
    struct root_ref* new = flup_list_first_entry(&self->cachedRootEntries, struct root_ref, node);
    flup_list_del(&new->node);
    return new;
  }
  
  return malloc(sizeof(struct root_ref));
}

void thread_new_root_ref_from_prealloc_no_gc_block(struct thread* self, struct root_ref* prealloc, struct alloc_unit* block) {
  prealloc->obj = block;
  self->rootSize++;
  flup_list_add_head(&self->rootEntries, &prealloc->node);
}


