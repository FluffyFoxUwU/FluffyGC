#include <mimalloc.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stddef.h>

#include <flup/bug.h>
#include <flup/data_structs/list_head.h>
#include <flup/data_structs/dyn_array.h>
#include <flup/concurrency/mutex.h>
#include <flup/core/logger.h>
#include <flup/util/min_max.h>

#include "alloc_tracker.h"
#include "memory/alloc_context.h"

#undef FLUP_LOG_CATEGORY
#define FLUP_LOG_CATEGORY "Alloc Tracker"

static void freeMemories(struct alloc_tracker* self) {
  flup_mutex_free(self->listOfContextLock);
  free(self);
}

struct alloc_tracker* alloc_tracker_new(size_t size) {
  struct alloc_tracker* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct alloc_tracker) {
    .currentUsage = 0,
    .maxSize = size,
    .contexts = FLUP_LIST_HEAD_INIT(self->contexts)
  };
  
  if (mi_reserve_os_memory_ex(
    size, 
    false, 
    false, 
    true, 
    &self->arena) != 0
  ) {
    pr_error("Failed to reserve memory for heap!");
    goto failure;
  }
  
  if (!(self->listOfContextLock = flup_mutex_new()))
    goto failure;
  
  return self;

failure:
  freeMemories(self);
  return NULL;
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
  
  freeMemories(self);
}

void alloc_tracker_filter_snapshot_and_delete_snapshot(struct alloc_tracker* self, struct alloc_tracker_snapshot* snapshot, alloc_tracker_snapshot_filter_func filter) {
  struct alloc_unit* next = snapshot->head;
  size_t freedSize = 0;
  while (next) {
    struct alloc_unit* current = next;
    next = next->next;
    
    if (filter(current)) {
      alloc_tracker_add_block_to_global_list(self, current);
      continue;
    }
    
    size_t totalSize = current->size + sizeof(*current);
    freedSize += totalSize;
    mi_free_size(current, totalSize);
  }
  atomic_fetch_sub_explicit(&self->currentUsage, freedSize, memory_order_relaxed);
  snapshot->head = NULL;
}

void alloc_tracker_add_block_to_global_list(struct alloc_tracker* self, struct alloc_unit* block) {
  struct alloc_unit* oldHead = atomic_load_explicit(&self->head, memory_order_relaxed);
  do {
    block->next = oldHead;
  } while (!atomic_compare_exchange_weak_explicit(&self->head, &oldHead, block, memory_order_release, memory_order_relaxed));
}


static bool slowDoLargeAccounting(struct alloc_tracker* self, struct alloc_context*, size_t accountSize) {
  size_t oldSize = atomic_load_explicit(&self->currentUsage, memory_order_relaxed);
  size_t newSize;
  do {
    if (oldSize + accountSize > self->maxSize)
      return false;
    
    newSize = oldSize + accountSize;
  } while (!atomic_compare_exchange_weak_explicit(&self->currentUsage, &oldSize, newSize, memory_order_release, memory_order_relaxed));
  atomic_fetch_add_explicit(&self->lifetimeBytesAllocated, accountSize, memory_order_relaxed);
  return true;
}

static bool fastDoSmallAccounting(struct alloc_tracker* self, struct alloc_context* ctx, size_t allocSize) {
  if (allocSize <= ctx->preReservedUsage)
    goto fast_accounted;
  
  if (!slowDoLargeAccounting(self, ctx, CONTEXT_COUNTER_PRERESERVE_SIZE))
    return false;
  
  ctx->preReservedUsage += CONTEXT_COUNTER_PRERESERVE_SIZE;
fast_accounted:
  ctx->preReservedUsage -= allocSize;
  return true;
}

struct alloc_unit* alloc_tracker_alloc(struct alloc_tracker* self, struct alloc_context* ctx, size_t allocSize) {
  struct alloc_unit* blockMetadata;
  
  blockMetadata = mi_heap_malloc(ctx->mimallocHeap, sizeof(*blockMetadata) + allocSize);
  if (!blockMetadata)
    return NULL;

  *blockMetadata = (struct alloc_unit) {
    .size = allocSize
  };
  
  size_t totalSize = allocSize + sizeof(struct alloc_unit);
  
  bool allocStatus;
  if (allocSize < CONTEXT_COUNTER_PRERESERVE_SKIP)
    allocStatus = fastDoSmallAccounting(self, ctx, totalSize);
  else
    allocStatus = slowDoLargeAccounting(self, ctx, totalSize);
  
  if (!allocStatus)
    goto failure;
  
  alloc_context_add_block(ctx, blockMetadata);
  return blockMetadata;

failure:
  mi_free_size(blockMetadata, totalSize);
  return NULL;
}

void alloc_tracker_take_snapshot(struct alloc_tracker* self, struct alloc_tracker_snapshot* snapshot) {
  flup_mutex_lock(self->listOfContextLock);
  *snapshot = (struct alloc_tracker_snapshot) {};
  
  __block struct alloc_unit* currentTail = NULL;
  auto appendToSnapshot = ^(struct alloc_unit* head, struct alloc_unit* tail) {
    if (snapshot->head == NULL) {
      // The head is null that mean its the first item, so lets set
      // head to current
      snapshot->head = head;
    } else {
      // The "head" not null which mean Foxie has to append current context's
      // list of allocated units to the tail of current snapshot's "head"
      
      // BUG_ON for some sanity check if tail indeed tail
      BUG_ON(currentTail->next != NULL);
      currentTail->next = head;
    }
    
    // Another BUG_ON for some sanity check if tail indeed tail
    BUG_ON(tail && tail->next != NULL);
    
    // Always set the tail to latest tail
    currentTail = tail;
  };
  
  struct flup_list_head* current;
  flup_list_for_each(&self->contexts, current) {
    struct alloc_context* ctx = flup_list_entry(current, struct alloc_context, node);
    if (!ctx->allocListHead)
      goto skip_this_context;
    appendToSnapshot(ctx->allocListHead, ctx->allocListTail);
    
    // Emptying the list in context as those invalid now
    ctx->allocListHead = NULL;
    ctx->allocListTail = NULL;
skip_this_context:
  }
  
  // Append current global list
  appendToSnapshot(atomic_exchange_explicit(&self->head, NULL, memory_order_relaxed), NULL);
  
  flup_mutex_unlock(self->listOfContextLock);
}

struct alloc_context* alloc_tracker_new_context(struct alloc_tracker* self) {
  struct alloc_context* ctx = alloc_context_new(self->arena);
  if (!ctx)
    return NULL;
  ctx->owner = self;
  
  flup_mutex_lock(self->listOfContextLock);
  flup_list_add_head(&self->contexts, &ctx->node);
  flup_mutex_unlock(self->listOfContextLock);
  return ctx;
}

void alloc_tracker_free_context(struct alloc_tracker* self, struct alloc_context* ctx) {
  flup_mutex_lock(self->listOfContextLock);
  flup_list_del(&ctx->node);
  alloc_context_free(self, ctx);
  flup_mutex_unlock(self->listOfContextLock);
}

void alloc_tracker_get_statistics(struct alloc_tracker* self, struct alloc_tracker_statistic* stat) {
  stat->maxSize = self->maxSize;
  stat->reservedBytes = stat->maxSize;
  stat->commitedBytes = stat->maxSize;
  stat->usedBytes = atomic_load_explicit(&self->currentUsage, memory_order_relaxed);
}

