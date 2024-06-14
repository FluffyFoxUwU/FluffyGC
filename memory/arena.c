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
    .currentUsage = 0,
    .maxSize = size
  };
  
  return self;
}

void arena_free(struct arena* self) {
  if (!self)
    return;
  
  arena_wipe(self);
  free(self);
}

static void freeBlock(struct arena_block* block) {
  free(block->data);
  free(block);
}

struct arena_block* arena_alloc(struct arena* self, size_t allocSize) {
  struct arena_block* blockMetadata;
  
  blockMetadata = malloc(sizeof(*blockMetadata));
  if (!blockMetadata)
    goto failure;

  *blockMetadata = (struct arena_block) {
    .size = allocSize
  };
  
  if (!(blockMetadata->data = malloc(allocSize)))
    goto failure;
  
  // Only add the block to array and update stats if ready to use
  // atomic_fetch_add(&self->currentUsage, totalSize);
  size_t totalSize = allocSize + sizeof(struct arena_block);
  size_t oldSize = atomic_load(&self->currentUsage);
  size_t newSize;
  do {
    if (oldSize + totalSize > self->maxSize)
      goto failure;
    
    newSize = oldSize + totalSize;
  } while (!atomic_compare_exchange_weak(&self->currentUsage, &oldSize, newSize));
  
  arena_move_one_block_from_detached_to_real_head(self, blockMetadata);
  return blockMetadata;

failure:
  freeBlock(blockMetadata);
  return NULL;
}

void arena_wipe(struct arena* self) {
  self->currentUsage = 0;
  
  // Detach head so can be independently wiped
  struct arena_block* current = arena_detach_head(self);
  struct arena_block* next;
  while (1) {
    next = atomic_load(&current->next);
    if (next == current) {
      arena_dealloc(self, current);
      break;
    }
    
    arena_dealloc(self, current);
    current = next;
  }
}

void arena_dealloc(struct arena* self, struct arena_block* blk) {
  atomic_fetch_sub(&self->currentUsage, blk->size + sizeof(*blk));
  free(blk->data);
  free(blk);
}

bool arena_is_end_of_detached_head(struct arena_block* blk) {
  return atomic_load(&blk->next) == blk;
}

void arena_move_one_block_from_detached_to_real_head(struct arena* self, struct arena_block* blk) {
  struct arena_block* oldHead = atomic_load(&self->head);
  do {
    if (oldHead == NULL)
      blk->next = blk;
    else
      blk->next = oldHead;
  } while (!atomic_compare_exchange_weak(&self->head, &oldHead, blk));
}

// Replace current head with NULL so alloc start new chain
struct arena_block* arena_detach_head(struct arena* self) {
  return atomic_exchange(&self->head, NULL);
}

