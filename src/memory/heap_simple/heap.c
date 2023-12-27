#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>

#include "attributes.h"
#include "concurrency/thread_local.h"
#include "heap.h"
#include "bug.h"
#include "heap_free_block_searchers.h"
#include "concurrency/mutex.h"
#include "object/object.h"
#include "util/list_head.h"
#include "util/util.h"

struct heap* heap_new(const struct heap_init_params* params) {
  struct heap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  *self = (struct heap) {};
  
  self->localHeapSize = ROUND_UP(params->localHeapSize, alignof(struct heap_block*));
  self->size = ROUND_UP(params->size, alignof(struct heap_block));
  self->base = malloc(self->size);
  self->bumpPointer = (uintptr_t) self->base;
  self->maxBumpPointer = (uintptr_t) self->base + (uintptr_t) self->size;
  if (!self->base)
    goto failure;
  
  atomic_init(&self->usage, 0);
  if (mutex_init(&self->lock) < 0)
    goto failure;
  if (thread_local_init(&self->localHeap, free) < 0)
    goto failure;
  
  list_head_init(&self->recentFreeBlocks);
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
  thread_local_cleanup(&self->localHeap);
  mutex_cleanup(&self->lock);
  free(self->base);
  free(self);
}

void* heap_block_get_ptr(struct heap_block* block) {
  return &block->data;
}

void heap_move(struct heap_block* src, struct heap_block* dest) {
  BUG_ON(src->blockSize != dest->blockSize);

  size_t copySize = src->blockSize - sizeof(*src);
  memcpy(&dest->data, &src->data, copySize);
}

// Try merge chain of free blocks
static void tryMergeFreeBlock(struct heap* self, struct heap_block* block) {
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &self->recentFreeBlocks) {
    struct heap_block* curBlock = list_entry(current, struct heap_block, node);
    
    // Free blocks isnt consecutive
    if (((char*) block) + block->blockSize != (char*) current)
      break;
    
    // Merge two blocks
    list_del(current);
    curBlock->blockSize += curBlock->blockSize;
  }
}

ATTRIBUTE_USED()
static void heap_merge_free_blocks(struct heap* self) {
  mutex_lock(&self->lock);
  
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &self->recentFreeBlocks)
    tryMergeFreeBlock(self, list_entry(current, struct heap_block, node));
  
  mutex_unlock(&self->lock);
}

static void heapDeallocNoLock(struct heap* self, struct heap_block* block) {
  // Remove from allocated block list
  list_del(&block->node);
  
  atomic_fetch_sub(&self->usage, block->blockSize);
  // printf("[Heap %p] Usage %10zu bytes / %10zu bytes (dealloc)\n", self, self->usage, self->size);
  
  list_add(&block->node, &self->recentFreeBlocks);
}

void heap_clear(struct heap* self) {
  mutex_lock(&self->lock);
  
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &self->allocatedBlocks)
    heapDeallocNoLock(self, list_entry(current, struct heap_block, node));
  
  // Resets allocation information
  list_head_init(&self->recentFreeBlocks);
  list_head_init(&self->allocatedBlocks);
  
  atomic_store(&self->bumpPointer, (uintptr_t) self->base);
  atomic_store(&self->usage, 0);
  mutex_unlock(&self->lock);
}

void heap_dealloc(struct heap* self, struct heap_block* block) {
  mutex_lock(&self->lock);
  heapDeallocNoLock(self, block);
  mutex_unlock(&self->lock);
}

static void initNewBlock(struct heap* self, struct heap_block* block, size_t blockSize) {
  *block = (struct heap_block) {
    .blockSize = blockSize
  };
  
  mutex_lock(&self->lock);
  list_add(&block->node, &self->allocatedBlocks);
  mutex_unlock(&self->lock);
   
  // printf("[Heap %p] Usage %10zu bytes / %10zu bytes (alloc)\n", self, self->usage, self->size);
}

struct local_heap_uwu {
  uintptr_t bumpPointer;
  uintptr_t endLocalHeap;
};

static inline struct local_heap_uwu* getLocalHeap(struct heap* self) {
  if (self->localHeapSize == 0)
    return NULL;
  
  struct local_heap_uwu* localHeap = thread_local_get(&self->localHeap);
  if (localHeap)
    return localHeap;
  
  localHeap = malloc(sizeof(*localHeap));
  if (!localHeap)
    return NULL;
  
  *localHeap = (struct local_heap_uwu) {};
  thread_local_set(&self->localHeap, localHeap);
  return localHeap;
}

static inline size_t getLocalHeapFreeSize(struct local_heap_uwu* self) {
  return self->endLocalHeap - self->bumpPointer;
}

// Allocate with alignment of heap_block
// `size` must strictly already aligned
static void* allocFastAligned(struct heap* self, size_t size) {
  BUG_ON(!IS_MULTIPLE_OF(size, alignof(struct heap_block)));

  void* block = NULL;
  if (!util_atomic_add_if_less_uintptr(&self->bumpPointer, size, self->maxBumpPointer, (uintptr_t*) &block))
    return NULL;
  return block;
}

static inline size_t calcHeapBlockSize(size_t objectSize) {
  return ROUND_UP(sizeof(struct heap_block) + objectSize, alignof(struct heap_block));
}

static struct heap_block* tryAllocFast(struct heap* self, size_t objectSize) {
  struct local_heap_uwu* localHeap = getLocalHeap(self);
  struct heap_block* block = NULL;
  size_t size = calcHeapBlockSize(objectSize);
   
  // Skip allocation from local heap if it larger
  // or non existent local heap
  if (size > self->localHeapSize || !localHeap) {
    if (!(block = allocFastAligned(self, size)))
      goto alloc_failure;
    goto alloc_sucess;
  }
  
  // Get new local heap (if its full)
  if (size > getLocalHeapFreeSize(localHeap)) {
    uintptr_t newLocalHeap = (uintptr_t) allocFastAligned(self, self->localHeapSize);
    if (!newLocalHeap)
      goto alloc_failure;
    localHeap->bumpPointer = newLocalHeap;
    localHeap->endLocalHeap = newLocalHeap + self->localHeapSize;
  }

  // Alloc from local heap
  block = (struct heap_block*) localHeap->bumpPointer;
  localHeap->bumpPointer += size;
alloc_sucess:
  initNewBlock(self, block, size);
alloc_failure:
  return block;
}

static struct heap_block* trySplitFreeBlock(struct heap_block* block, size_t blockSize) {
  if (block->blockSize == blockSize)
    return block;

  // Bug in heap free block searcher
  BUG_ON(blockSize > block->blockSize);
  
  // Not enough space for new block
  // Because the second block must fit
  // the new block
  if (block->blockSize - blockSize < sizeof(struct heap_block))
    return block;
  
  struct heap_block* blockA = block;
  struct heap_block* blockB = (struct heap_block*) ((char*) block + blockSize);
  
  blockSize = (uintptr_t) blockB - (uintptr_t) blockA;
  
  blockB->blockSize = block->blockSize - blockSize;
  list_add(&blockB->node, &blockA->node);
  return block;
}

struct heap_block* heap_alloc(struct heap* self, size_t objectSize) {
  if (objectSize == 0)
    return NULL;
  
  struct heap_block* block = NULL;
  size_t blockSize = calcHeapBlockSize(objectSize); 
  
  // Check if out of memory or not
  bool isOOM = !util_atomic_add_if_less_size_t(&self->usage, blockSize, self->size, NULL);
  if (isOOM) {
    // printf("[Heap %p] Out of memory: Usage %10zu bytes / %10zu bytes (alloc)\n", self, self->usage, self->size);
    return NULL;
  }

  if ((block = tryAllocFast(self, objectSize)))
    return block;
  
  // Slow alloc method
  mutex_lock(&self->lock);
  block = heap_find_free_block(self, blockSize);
  if (!block) 
    goto alloc_failed;
  
  block = trySplitFreeBlock(block, blockSize);
  
  // Remove from free block list
  list_del(&block->node);
  
  initNewBlock(self, block, blockSize);
alloc_failed:
  mutex_unlock(&self->lock);
  return block;
}

