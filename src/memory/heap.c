#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <errno.h>

#include "concurrency/thread_local.h"
#include "heap.h"
#include "bug.h"
#include "config.h"
#include "heap_free_block_searchers.h"
#include "concurrency/mutex.h"
#include "object/object.h"
#include "util/list_head.h"
#include "util/util.h"

struct heap* heap_new(size_t size) {
  void* ptr = malloc(size);
  if (!ptr)
    return NULL;
  return heap_from_existing(size, ptr, free);
}

struct heap* heap_from_existing(size_t size, void* ptr, void (*destroyer)(void*)) {
  struct heap* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  *self = (struct heap) {};
  size = ROUND_UP(size, alignof(struct heap_block));
  
  self->base = ptr;
  self->destroyerUwU = destroyer;
  self->bumpPointer = (uintptr_t) ptr;
  self->maxBumpPointer = (uintptr_t) ptr + size;
  self->size = size;
  atomic_init(&self->usage, 0);
  if (mutex_init(&self->lock) < 0)
    goto failure;
  if (thread_local_init(&self->localHeapKey, free) < 0)
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
  thread_local_cleanup(&self->localHeapKey);
  mutex_cleanup(&self->lock);
  self->destroyerUwU(self->base);
  free(self);
}

void heap_init(struct heap* self) {
  self->initialized = true;
}

int heap_param_set_local_heap_size(struct heap* self, size_t size) {
  self->localHeapSize = alignof(typeof(size));
  return 0;
}

// Try merge chain of free blocks
static void tryMergeFreeBlock(struct heap* self, struct heap_block* block) {
  BUG_ON(!block->isFree);
  
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &self->recentFreeBlocks) {
    struct heap_block* block = list_entry(current, struct heap_block, node);
    BUG_ON(!block->isFree);
    // Free blocks isnt consecutive
    if (((void*) block) + block->blockSize != current)
      break;
    
    // Merge two blocks
    list_del(current);
    block->blockSize += block->blockSize;
  }
}

void heap_merge_free_blocks(struct heap* self) {
  mutex_lock(&self->lock);
  
  struct list_head* current;
  struct list_head* next;
  list_for_each_safe(current, next, &self->recentFreeBlocks)
    tryMergeFreeBlock(self, list_entry(current, struct heap_block, node));
  
  mutex_unlock(&self->lock);
}

static void initFreeBlock(struct heap* self, struct heap_block* block, size_t blockSize) {
  *block = (struct heap_block) {
    .isFree = true,
    .blockSize = blockSize,
    .alignment = alignof(typeof(*block))
  };
}

static void postFreeHook(struct heap* self, struct heap_block* block) {
  gc_current->hooks->preObjectDealloc(&block->objMetadata);
  atomic_fetch_sub(&self->usage, block->blockSize);
  // printf("[Heap %p] Usage %10zu bytes / %10zu bytes (dealloc)\n", self, self->usage, self->size);
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    free(block->dataPtr.ptr);
  block->dataPtr = USERPTR_NULL;
}

static void heapDeallocNoLock(struct heap* self, struct heap_block* block) {
  // Double dealloc
  BUG_ON(block->isFree);
  
  // Remove from allocated block list
  list_del(&block->node);
  
  postFreeHook(self, block);
  initFreeBlock(self, block, block->blockSize);
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    free(block);
  else
    list_add(&block->node, &self->recentFreeBlocks);
}

void heap_dealloc(struct heap* self, struct heap_block* block) {
  mutex_lock(&self->lock);
  heapDeallocNoLock(self, block);
  mutex_unlock(&self->lock);
}

static inline struct heap_block* fixAlignment(struct heap_block* block) {
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    return block;
  return PTR_ALIGN(block, alignof(struct heap_block));
}

static struct heap_block* splitFreeBlocks(struct heap* self, struct heap_block* block, size_t size) {
  if (block->blockSize == size)
    return block;
  BUG_ON(size > block->blockSize);
  
  // Not enough space for new block
  // Because the second block must fit the new block
  if (block->blockSize - size < sizeof(struct heap_block))
    return block;
  
  struct heap_block* blockA = block;
  struct heap_block* blockB = ((void*) block) + size;
  
  // Re-align
  blockB = fixAlignment(blockB);
  size = (uintptr_t) blockB - (uintptr_t) blockA;
  
  initFreeBlock(self, blockB, block->blockSize - size);
  
  list_add(&blockB->node, &blockA->node);
  return block;
}

struct local_heap_uwu {
  uintptr_t bumpPointer;
  uintptr_t endLocalHeap;
};

static inline size_t getLocalHeapSize(struct local_heap_uwu* self) {
  return self->endLocalHeap - self->bumpPointer;
}

static inline struct local_heap_uwu* getLocalHeap(struct heap* self) {
  if (self->localHeapSize == 0)
    return NULL;
  
  struct local_heap_uwu* localHeap = thread_local_get(&self->localHeapKey);
  if (localHeap)
    return localHeap;
  
  localHeap = malloc(sizeof(*localHeap));
  if (!localHeap)
    return NULL;
  
  *localHeap = (struct local_heap_uwu) {};
  thread_local_set(&self->localHeapKey, localHeap);
  return localHeap;
}

static struct heap_block* commonBlockInit(struct heap* self, struct heap_block* block, size_t dataAlignment, size_t blockSize, size_t objectSize, bool isSlowAlloc) {
  if (!IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    block = fixAlignment(block);
  
  bool isOOM = !util_atomic_add_if_less_size_t(&self->usage, blockSize, self->size, NULL);
  if (isOOM) {
    BUG_ON(!IS_ENABLED(CONFIG_HEAP_USE_MALLOC));
     // printf("[Heap %p] Out of memory: Usage %10zu bytes / %10zu bytes (alloc)\n", self, self->usage, self->size);
    return NULL;
  }
  
  *block = (struct heap_block) {
    .isFree = false,
    .blockSize = blockSize,
    .dataSize = objectSize,
    .alignment = MAX(alignof(struct heap_block), dataAlignment),
  };
  block->dataPtr = USERPTR(PTR_ALIGN(&block->data, block->alignment));
  
  mutex_lock(&self->lock);
  list_add(&block->node, &self->allocatedBlocks);
  mutex_unlock(&self->lock);
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    block->dataPtr = USERPTR(aligned_alloc(block->alignment, block->dataSize));
    if (!block->dataPtr.ptr)
      goto failure;
  } else {
    block->dataPtr.ptr = PTR_ALIGN(&block->data, dataAlignment);
  }
  
  // printf("[Heap %p] Usage %10zu bytes / %10zu bytes (alloc)\n", self, self->usage, self->size);
  
  // Fast alloc dont lock the heap
  // and GC hooks need to be called with no lock held on heap
  if (gc_current->hooks->postObjectAlloc(&block->objMetadata) < 0)
    goto failure;
  return block;

failure:
  heap_dealloc(self, block);
  return NULL;
}

// Also compensate for alignment
// by giving extra space
static inline size_t calcHeapBlockSize(size_t alignment, size_t objectSize) {
  return ROUND_UP(sizeof(struct heap_block) + ROUND_UP(objectSize, alignment), alignof(struct heap_block));
}

static void* allocFastRaw(struct heap* self, size_t size) {
  void* block = NULL;
  if (!util_atomic_add_if_less_uintptr(&self->bumpPointer, size, self->maxBumpPointer, (uintptr_t*) &block))
    return NULL;
  return block;
}

struct heap_block* heap_alloc_fast(struct heap* self, size_t dataAlignment, size_t objectSize) {
  struct local_heap_uwu* localHeap = getLocalHeap(self);
  struct heap_block* block = NULL;
  size_t size = calcHeapBlockSize(dataAlignment, objectSize);
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    block = malloc(sizeof(*block));
    goto alloc_sucess;
  }
  
  // Skip allocation from local heap
  if (size > self->localHeapSize || !localHeap) {
    if (!(block = allocFastRaw(self, size)))
      goto alloc_failure;
    goto alloc_sucess;
  }
  
  // Get new local heap (cant fit)
  if (size > getLocalHeapSize(localHeap)) {
    uintptr_t newLocalHeap = (uintptr_t) allocFastRaw(self, self->localHeapSize);
    if (!newLocalHeap)
      goto alloc_failure;
    localHeap->bumpPointer = newLocalHeap;
    localHeap->endLocalHeap = newLocalHeap + self->localHeapSize;
  }

  block = (struct heap_block*) localHeap->bumpPointer;
  localHeap->bumpPointer += size;
alloc_sucess:;
  struct heap_block* computedBlock = commonBlockInit(self, block, dataAlignment, size, objectSize, false);
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC) && computedBlock == NULL)
    free(block);
  block = computedBlock;
alloc_failure:
  return block;
}

struct heap_block* heap_alloc(struct heap* self, size_t dataAlignment, size_t objectSize) {
  if (objectSize == 0)
    return NULL;
  struct heap_block* block = NULL;
  
  if ((block = heap_alloc_fast(self, dataAlignment, objectSize)))
    return block;
  
  // The rest of the logic not applicable for malloc based
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    return NULL;
  
  size_t size = calcHeapBlockSize(dataAlignment, objectSize);
  
  // Slow alloc method
  mutex_lock(&self->lock);
  block = heap_find_free_block(self, size);
  if (!block) 
    goto alloc_failed;
  
  block = splitFreeBlocks(self, block, size);
  
  // Remove from free block list
  list_del(&block->node);
  
alloc_failed:
  mutex_unlock(&self->lock);
  if (block)
    block = commonBlockInit(self, block, dataAlignment, size, objectSize, true);
  return block;
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

