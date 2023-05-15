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
#include "gc/gc.h"

thread_local struct context* context_current = NULL;

SOC_DEFINE(listNodeCache, SOC_DEFAULT_CHUNK_SIZE, struct root_ref)

struct context* context_new() {
  struct context* self = malloc(sizeof(*self)); 
  if (!self)
    return NULL;
  *self = (struct context) {};
  
  list_head_init(&self->root);
  return self;
}

void context_free(struct context* self) {
  if (gc_current->hooks->preContextCleanup(self) < 0)
    BUG();
  
  struct list_head* current;
  struct list_head* n;
  list_for_each_safe(current, n, &self->root)
    list_del(current);
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
  struct root_ref* rootRef = soc_alloc(listNodeCache);
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
  soc_dealloc(listNodeCache, obj);
  context_unblock_gc();
  return 0;
}

