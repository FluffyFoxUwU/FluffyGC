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

struct alloc_tracker* arena_new(size_t size) {
  struct alloc_tracker* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct alloc_tracker) {
    .currentUsage = 0,
    .maxSize = size
  };
  
  return self;
}

void arena_free(struct alloc_tracker* self) {
  if (!self)
    return;
  
  arena_wipe(self);
  free(self);
}

static void freeBlock(struct alloc_unit* block) {
  free(block->data);
  free(block);
}

struct alloc_unit* arena_alloc(struct alloc_tracker* self, size_t allocSize) {
  struct alloc_unit* blockMetadata;
  
  blockMetadata = malloc(sizeof(*blockMetadata));
  if (!blockMetadata)
    goto failure;

  *blockMetadata = (struct alloc_unit) {
    .size = allocSize
  };
  
  if (!(blockMetadata->data = malloc(allocSize)))
    goto failure;
  
  // Only add the block to array and update stats if ready to use
  // atomic_fetch_add(&self->currentUsage, totalSize);
  size_t totalSize = allocSize + sizeof(struct alloc_unit);
  size_t oldSize = atomic_load(&self->currentUsage);
  size_t newSize;
  do {
    if (oldSize + totalSize > self->maxSize)
      goto failure;
    
    newSize = oldSize + totalSize;
  } while (!atomic_compare_exchange_weak(&self->currentUsage, &oldSize, newSize));
  
  atomic_fetch_add(&self->metadataUsage, sizeof(struct alloc_unit));
  atomic_fetch_add(&self->nonMetadataUsage, allocSize);
  atomic_fetch_add(&self->lifetimeBytesAllocated, totalSize);
  arena_move_one_block_from_detached_to_real_head(self, blockMetadata);
  return blockMetadata;

failure:
  freeBlock(blockMetadata);
  return NULL;
}

void arena_wipe(struct alloc_tracker* self) {
  self->currentUsage = 0;
  
  // Detach head so can be independently wiped
  struct alloc_unit* current = arena_detach_head(self);
  struct alloc_unit* next;
  while (current) {
    next = current->next;
    arena_dealloc(self, current);
    current = next;
  }
}

void arena_dealloc(struct alloc_tracker* self, struct alloc_unit* blk) {
  atomic_fetch_sub(&self->currentUsage, blk->size + sizeof(*blk));
  atomic_fetch_sub(&self->metadataUsage, sizeof(*blk));
  atomic_fetch_sub(&self->nonMetadataUsage, blk->size);
  free(blk->data);
  free(blk);
}

bool arena_is_end_of_detached_head(struct alloc_unit* blk) {
  return blk->next == NULL;
}

void arena_move_one_block_from_detached_to_real_head(struct alloc_tracker* self, struct alloc_unit* blk) {
  struct alloc_unit* oldHead = atomic_load(&self->head);
  do {
    blk->next = oldHead;
  } while (!atomic_compare_exchange_weak(&self->head, &oldHead, blk));
}

// Replace current head with NULL so alloc start new chain
struct alloc_unit* arena_detach_head(struct alloc_tracker* self) {
  return atomic_exchange(&self->head, NULL);
}

