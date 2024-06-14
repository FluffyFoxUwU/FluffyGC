#include <flup/data_structs/list_head.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

#include <flup/bug.h>
#include <flup/data_structs/dyn_array.h>
#include <flup/concurrency/mutex.h>
#include <flup/core/logger.h>
#include <flup/util/min_max.h>

#include "arena.h"

struct arena* arena_new(size_t size) {
  struct arena* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct arena) {
    .stats.currentUsage = 0,
    .stats.maxSize = size,
    .blocks = NULL,
    .lock = NULL,
    .freeList = FLUP_LIST_HEAD_INIT(self->freeList),
    // How many blocks can store accounting for metadata + a pointer to it
    .stats.maxBlocksCount = size / (sizeof(struct arena_block) + sizeof(void*))
  };
  
  if ((self->lock = flup_mutex_new()) == NULL)
    goto failure;
  
  pr_info("Maximum blocks can be sotored is %zu", self->stats.maxBlocksCount);
  if (!(self->blocks = calloc(self->stats.maxBlocksCount, sizeof(void*))))
    goto failure;
  self->stats.currentUsage += self->stats.maxBlocksCount * sizeof(void*);
  return self;

failure:
  arena_free(self);
  return NULL;
}

void arena_free(struct arena* self) {
  if (!self)
    return;
  
  arena_wipe(self);
  free(self->blocks);
  flup_mutex_free(self->lock);
  free(self);
}

static void freeBlock(struct arena_block* block) {
  free(block->data);
  free(block);
}

struct arena_block* arena_alloc(struct arena* self, size_t allocSize) {
  flup_mutex_lock(self->lock);
  size_t totalSize = allocSize;
  // Include metadata size too if its going to be allocated
  if (flup_list_is_empty(&self->freeList))
    totalSize += sizeof(struct arena_block);
  
  if (self->stats.currentUsage + totalSize > self->stats.maxSize) {
    flup_mutex_unlock(self->lock);
    return NULL;
  }
  
  struct arena_block* blockMetadata;
  size_t blockIndex;
  
  if (!flup_list_is_empty(&self->freeList)) {
    blockMetadata = flup_list_first_entry(&self->freeList, struct arena_block, node);
    blockIndex = blockMetadata->index;
    flup_list_del(&blockMetadata->node);
    goto free_block_exist;
  }
  
  // Could there miscalculation? of the size for blocks array?
  BUG_ON(self->numBlocksCreated == self->stats.maxBlocksCount);
  
  blockMetadata = malloc(sizeof(*blockMetadata));
  if (!blockMetadata)
    goto failure;
  
  blockIndex = self->numBlocksCreated;
  self->numBlocksCreated++;
free_block_exist:
  
  *blockMetadata = (struct arena_block) {
    .size = allocSize,
    .index = blockIndex
  };
  
  if (!(blockMetadata->data = malloc(allocSize)))
    goto failure;
  
  // Only add the block to array and update stats if ready to use
  atomic_store(&self->blocks[blockMetadata->index], blockMetadata);
  self->stats.currentUsage += totalSize;
  flup_mutex_unlock(self->lock);
  return blockMetadata;

failure:
  freeBlock(blockMetadata);
  flup_mutex_unlock(self->lock);
  return NULL;
}

void arena_wipe(struct arena* self) {
  flup_mutex_lock(self->lock);
  self->stats.currentUsage = 0;
  
  for (size_t i = 0; i < self->stats.maxBlocksCount; i++) {
    struct arena_block* block = self->blocks[i];
    if (!block)
      continue;
    self->stats.currentUsage -= block->size + sizeof(*block);
    freeBlock(block);
  }
  
  flup_list_head* current;
  flup_list_head* next;
  flup_list_for_each_safe(&self->freeList, current, next) {
    flup_list_del(current);
    freeBlock(flup_list_entry(current, struct arena_block, node));
  }
  
  // Clear the array
  memset(self->blocks, 0, sizeof(void*) * self->stats.maxBlocksCount);
  self->numBlocksCreated = 0;
  flup_mutex_unlock(self->lock);
}

void arena_dealloc(struct arena* self, struct arena_block* blk) {
  flup_mutex_lock(self->lock);
  free(blk->data);
  blk->data = NULL;
  
  atomic_store(&self->blocks[blk->index], NULL);
  self->stats.currentUsage -= blk->size;
  flup_list_add_head(&self->freeList, &blk->node);
  flup_mutex_unlock(self->lock);
}

void arena_get_stat(struct arena* self, struct arena_stats* stats) {
  flup_mutex_lock(self->lock);
  *stats = self->stats;
  flup_mutex_unlock(self->lock);
}

