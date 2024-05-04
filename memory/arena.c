#include <flup/data_structs/list_head.h>
#include <stdlib.h>
#include <unistd.h>
#include <stddef.h>

#include <flup/bug.h>
#include <flup/data_structs/dyn_array.h>
#include <flup/concurrency/mutex.h>
#include <flup/core/logger.h>

#include "arena.h"

struct arena* arena_new(size_t size) {
  struct arena* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct arena) {
    .currentUsage = 0,
    .maxSize = size,
    .blocks = NULL,
    .lock = NULL,
    .freeList = FLUP_LIST_HEAD_INIT(self->freeList)
  };
  
  if ((self->lock = flup_mutex_new()) == NULL)
    goto failure;
  
  if (!(self->blocks = flup_dyn_array_new(sizeof(void*), 0)))
    goto failure;
  
  return self;

failure:
  arena_free(self);
  return NULL;
}

void arena_free(struct arena* self) {
  if (!self)
    return;
  
  arena_wipe(self);
  flup_dyn_array_free(self->blocks);
  flup_mutex_free(self->lock);
  free(self);
}

static void freeBlock(struct arena_block* block) {
  if (!block)
    return;
  
  free(block->data);
  free(block);
}

struct arena_block* arena_alloc(struct arena* self, size_t size) {
  flup_mutex_lock(self->lock);
  if (self->currentUsage + size > self->maxSize) {
    flup_mutex_unlock(self->lock);
    return NULL;
  }
  self->currentUsage += size;
  
  struct arena_block* block;
  
  if (!flup_list_is_empty(&self->freeList)) {
    block = flup_list_first_entry(&self->freeList, struct arena_block, node);
    flup_list_del(&block->node);
    goto free_block_exist;
  }
  
  block = malloc(sizeof(*block));
  if (!block)
    goto failure;
free_block_exist:
  
  *block = (struct arena_block) {
    .used = true
  };
  
  if (!(block->data = malloc(size)))
    goto failure;
  
  if (flup_dyn_array_append(self->blocks, &block) < 0)
    goto failure;
  flup_mutex_unlock(self->lock);
  return block;

failure:
  self->currentUsage -= size;
  freeBlock(block);
  flup_mutex_unlock(self->lock);
  return NULL;
}

void arena_wipe(struct arena* self) {
  flup_mutex_lock(self->lock);
  self->currentUsage = 0;
  
  for (size_t i = 0; i < self->blocks->length; i++) {
    struct arena_block** block;
    bool ret = flup_dyn_array_get(self->blocks, i, (void**) &block);
    BUG_ON(!ret);
    
    freeBlock(*block);
  }
  
  // Clear the array
  flup_dyn_array_remove(self->blocks, 0, self->blocks->length);
  flup_mutex_unlock(self->lock);
}

void arena_dealloc(struct arena* self, struct arena_block* blk) {
  flup_mutex_lock(self->lock);
  free(blk->data);
  blk->data = NULL;
  blk->used = false;
  
  flup_list_add_head(&self->freeList, &blk->node);
  flup_mutex_unlock(self->lock);
}

