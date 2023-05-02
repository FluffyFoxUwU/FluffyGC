#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <threads.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>

#include "concurrency/completion.h"
#include "concurrency/event.h"
#include "concurrency/rwlock.h"
#include "memory/soc.h"
#include "context.h"
#include "bug.h"
#include "managed_heap.h"
#include "object/object.h"
#include "util/list_head.h"
#include "util/refcount.h"
#include "util/util.h"

thread_local struct context* context_current = NULL;

struct context* context_new() {
  struct context* self = malloc(sizeof(*self)); 
  if (!self)
    return NULL;
  *self = (struct context) {};
  
  self->listNodeCache = soc_new(sizeof(struct root_ref), 0);
  if (!self->listNodeCache)
    goto failure;
  
  list_head_init(&self->root);
  return self;
  
failure:
  context_free(self);
  return NULL;
}

void context_free(struct context* self) {
  // No need to clear each root entry this nukes all associated with current
  soc_free(self->listNodeCache);
  free(self);
}

void context__block_gc() {
  unsigned int last = atomic_fetch_add(&context_current->blockCount, 1);
  BUG_ON(last >= UINT_MAX);
  
  if (last == 0 && context_current->managedHeap)
    managed_heap_block_gc(context_current->managedHeap);
}

void context__unblock_gc() {
  unsigned int last = atomic_fetch_sub(&context_current->blockCount, 1);
  BUG_ON(last >= UINT_MAX);
  
  if (last == 1 && context_current->managedHeap)
    managed_heap_unblock_gc(context_current->managedHeap);
}

void context_add_pinned_object(struct root_ref* obj) {
  refcount_acquire(&obj->pinCounter);
}

void context_remove_pinned_object(struct root_ref* obj) {
  refcount_release(&obj->pinCounter);
}

struct root_ref* context_add_root_object(struct object* obj) {
  context_block_gc();
  struct root_ref* rootRef = soc_alloc(context_current->listNodeCache);
  if (!rootRef)
    return NULL;
  
  *rootRef = (struct root_ref) {};
  atomic_init(&rootRef->obj, obj);
  refcount_init(&rootRef->pinCounter);
  
  list_add(&rootRef->node, &context_current->root);
  context_unblock_gc();
  return rootRef;
}

int context_remove_root_object(struct root_ref* obj) {
  context_block_gc();
  list_del(&obj->node);
  soc_dealloc(context_current->listNodeCache, obj);
  context_unblock_gc();
  return 0;
}

