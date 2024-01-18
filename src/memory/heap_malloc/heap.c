#include <stddef.h>
#include <stdlib.h>

#include "bug.h"
#include "memory/heap_common_types.h"
#include "util/list_head.h"
#include "concurrency/mutex.h"

#include "util/util.h"
#include "heap.h"

void* heap_block_get_ptr(struct heap_block* block) {
  return block->allocPtr;
}

void heap_move(struct heap_block* src, struct heap_block* dest) {
  BUG_ON(src->blockSize != dest->blockSize);
  dest->allocPtr = src->allocPtr;
}

struct heap_block* heap_alloc(struct heap* self, size_t size) {
  mutex_lock(&self->lock);
  size = ROUND_UP(size, alignof(max_align_t));
  struct heap_block* block = NULL;
  
  // Out of memory
  if (self->totalSize - self->usage < size)
    goto failure;

  block = malloc(sizeof(*block));
  void* data = malloc(size);

  if (!block || !data) {
    free(data);
    free(block);
    goto failure;
  }
  
  self->usage += size;
  block->allocPtr = data;
  list_add(&block->node, &self->allocatedBlocks);
failure:
  mutex_unlock(&self->lock);
  return block;
}

static void deallocNoLock(struct heap_block* block) {
  list_del(&block->node);
  free(block->allocPtr);
  free(block);
}

void heap_dealloc(struct heap* self, struct heap_block* block) {
  mutex_lock(&self->lock);
  deallocNoLock(block);
  mutex_unlock(&self->lock);
}

void heap_clear(struct heap* self) {
  mutex_lock(&self->lock);
  
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &self->allocatedBlocks)
    deallocNoLock(list_entry(current, struct heap_block, node));
  self->usage = 0;
  
  mutex_unlock(&self->lock);
}

struct heap* heap_new(const struct heap_init_params* params) {
  struct heap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;

  self->usage = 0;
  self->totalSize = params->size;
  if (mutex_init(&self->lock) < 0)
    goto failure;
  list_head_init(&self->allocatedBlocks);
  return self;

failure:
  heap_free(self);
  return NULL;
}

void heap_free(struct heap* self) {
  if (!self)
    return;

  heap_clear(self);
  mutex_cleanup(&self->lock);
  free(self);
}



