#include <stdlib.h>
#include <stddef.h>

#include <flup/bug.h>
#include <flup/concurrency/mutex.h>
#include <flup/container_of.h>
#include <flup/core/logger.h>
#include <flup/data_structs/list_head.h>

#include "heap.h"
#include "gc/gc.h"
#include "heap/generation.h"
#include "memory/arena.h"

#define HEAP_ALLOC_RETRY_COUNT 5

struct heap* heap_new(size_t size) {
  struct heap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct heap) {
    .root = FLUP_LIST_HEAD_INIT(self->root),
    .rootEntryCount = 0
  };
  
  if (!(self->gen = generation_new(size)))
    goto failure;
  self->gen->ownerHeap = self;
  return self;

failure:
  heap_free(self);
  return NULL;
}

void heap_free(struct heap* self) {
  flup_list_head* current;
  flup_list_head* next;
  flup_list_for_each_safe(&self->root, current, next)
    heap_root_unref(self, container_of(current, struct root_ref, node));
  generation_free(self->gen);
  free(self);
}

struct root_ref* heap_root_dup(struct heap* self, struct root_ref* ref) {
  struct root_ref* newRef = malloc(sizeof(*self));
  if (!newRef)
    return NULL;
  
  heap_block_gc(self);
  flup_list_add_tail(&self->root, &newRef->node);
  newRef->obj = ref->obj;
  self->rootEntryCount++;
  heap_unblock_gc(self);
  return newRef;
}

void heap_root_unref(struct heap* self, struct root_ref* ref) {
  heap_block_gc(self);
  self->rootEntryCount--;
  flup_list_del(&ref->node);
  
  gc_on_reference_lost(ref->obj);
  heap_unblock_gc(self);
  free(ref);
}

struct root_ref* heap_alloc(struct heap* self, size_t size) {
  struct root_ref* ref = malloc(sizeof(*ref));
  if (!ref)
    return NULL;
  
  heap_block_gc(self);
  struct arena_block* newObj = generation_alloc(self->gen, size);
  for (int i = 0; i < HEAP_ALLOC_RETRY_COUNT && newObj == NULL; i++) {
    pr_info("Allocation failed trying calling GC #%d", i + 1);
    heap_unblock_gc(self);
    gc_start_cycle(self->gen->gcState);
    heap_block_gc(self);
    newObj = generation_alloc(self->gen, size);
  }
  
  // Heap is actually OOM-ed
  if (!newObj) {
    free(ref);
    heap_unblock_gc(self);
    return NULL;
  }
  
  flup_list_add_tail(&self->root, &ref->node);
  ref->obj = newObj;
  self->rootEntryCount++;
  heap_unblock_gc(self);
  return ref;
}

void heap_block_gc(struct heap* self) {
  gc_block(self->gen->gcState);
}

void heap_unblock_gc(struct heap* self) {
  gc_unblock(self->gen->gcState);
}


