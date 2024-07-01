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

#include "alloc_tracker.h"

struct alloc_tracker* alloc_tracker_new(size_t size) {
  struct alloc_tracker* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct alloc_tracker) {
    .currentUsage = 0,
    .maxSize = size
  };
  
  return self;
}

void alloc_tracker_free(struct alloc_tracker* self) {
  if (!self)
    return;
  
  // Riding on the behaviour lol
  struct alloc_tracker_snapshot snapshot;
  alloc_tracker_take_snapshot(self, &snapshot);
  alloc_tracker_filter_snapshot_and_delete_snapshot(self, &snapshot, ^bool(struct alloc_unit*) {
    return false;
  });
  
  free(self);
}

static void deallocBlock(struct alloc_tracker* self, struct alloc_unit* blk);
static void addBlock(struct alloc_tracker* self, struct alloc_unit* block);
void alloc_tracker_filter_snapshot_and_delete_snapshot(struct alloc_tracker* self, struct alloc_tracker_snapshot* snapshot, alloc_tracker_snapshot_filter_func filter) {
  struct alloc_unit* next = snapshot->head;
  while (next) {
    struct alloc_unit* current = next;
    next = next->next;
    
    if (filter(current))
      addBlock(self, current);
    else
      deallocBlock(self, current);
  }
  snapshot->head = NULL;
}

static void freeBlock(struct alloc_unit* block) {
  free(block->data);
  free(block);
}

static void addBlock(struct alloc_tracker* self, struct alloc_unit* block) {
  struct alloc_unit* oldHead = atomic_load(&self->head);
  do {
    block->next = oldHead;
  } while (!atomic_compare_exchange_weak(&self->head, &oldHead, block));
}

struct alloc_unit* alloc_tracker_alloc(struct alloc_tracker* self, size_t allocSize) {
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
  addBlock(self, blockMetadata);
  return blockMetadata;

failure:
  freeBlock(blockMetadata);
  return NULL;
}

static void deallocBlock(struct alloc_tracker* self, struct alloc_unit* blk) {
  atomic_fetch_sub(&self->currentUsage, blk->size + sizeof(*blk));
  atomic_fetch_sub(&self->metadataUsage, sizeof(*blk));
  atomic_fetch_sub(&self->nonMetadataUsage, blk->size);
  free(blk->data);
  free(blk);
}

void alloc_tracker_take_snapshot(struct alloc_tracker* self, struct alloc_tracker_snapshot* detached) {
  detached->head = atomic_exchange(&self->head, NULL);
}

struct alloc_context* alloc_tracker_new_context(struct alloc_tracker* self) {
  struct alloc_context* ctx = malloc(sizeof(*ctx));
  if (!ctx)
    return NULL;
  
  *ctx = (struct alloc_context) {};
  
  if (!(ctx->contextLock = flup_mutex_new())) {
    alloc_tracker_free_context(self, ctx);
    return NULL;
  }
  return ctx;
}

void alloc_tracker_free_context(struct alloc_tracker* self, struct alloc_context* ctx) {
  if (!self)
    return;
  
  flup_mutex_lock(ctx->contextLock);
  
  struct alloc_unit* next = ctx->allocListHead;
  while (next) {
    struct alloc_unit* current = next;
    next = next->next;
    
    addBlock(self, current);
  }
  
  flup_mutex_unlock(ctx->contextLock);
  flup_mutex_free(ctx->contextLock);
  free(ctx);
}

