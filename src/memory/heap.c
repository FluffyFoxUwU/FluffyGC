#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <errno.h>

#include "concurrency/thread_local.h"
#include "context.h"
#include "gc/gc.h"
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
  size = ROUND_DOWN(size, alignof(struct heap_block));
  
  self->pool = ptr;
  self->destroyerUwU = destroyer;
  self->bumpPointer = (uintptr_t) ptr;
  self->maxBumpPointer = (uintptr_t) ptr + size;
  self->size = size;
  if (mutex_init(&self->lock) < 0)
    goto failure;
  if (thread_local_init(&self->localHeapKey, free) < 0)
    goto failure;
  
  list_head_init(&self->recentFreeBlocks);
  list_head_init(&self->recentAllocatedBlocks);
  return self;
failure:
  heap_free(self);
  return NULL;
}

void heap_free(struct heap* self) {
  thread_local_cleanup(&self->localHeapKey);
  mutex_cleanup(&self->lock);
  self->destroyerUwU(self->pool);
  free(self);
}

void heap_init(struct heap* self) {
  self->initialized = true;
}

int heap_param_set_local_heap_size(struct heap* self, size_t size) {
  self->localHeapSize = alignof(typeof(size));
  return 0;
}

static void initAllocatedBlock(struct heap* self, struct heap_block* block, size_t dataAlignment, size_t blockSize, size_t objectSize) {
  *block = (struct heap_block) {
    .isFree = false,
    .blockSize = blockSize,
    .dataSize = objectSize,
    .alignment = MAX(alignof(struct heap_block), dataAlignment)
  };
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
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    free(block->dataPtr.ptr);
  block->dataPtr = USERPTR_NULL;
}

static int postAllocHook(struct heap* self, struct heap_block* block) {
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    block->dataPtr = USERPTR(aligned_alloc(block->alignment, block->dataSize));
    if (!block->dataPtr.ptr)
      return -ENOMEM;
  } else {
    block->dataPtr = USERPTR(PTR_ALIGN(&block->data, block->alignment));
  }
  
  return gc_current->hooks->postObjectAlloc(&block->objMetadata);
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

void heap_dealloc(struct heap* self, struct heap_block* block) {
  mutex_lock(&self->lock);
  
  // Double dealloc
  BUG_ON(block->isFree);
  
  postFreeHook(self, block);
  initFreeBlock(self, block, block->blockSize);
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    free(block->dataPtr.ptr);
    free(block);
    goto skip;
  }
  
  // Add to free list
  list_add(&block->node, &self->recentFreeBlocks);
skip:
  mutex_unlock(&self->lock);
}

static void* allocFastRaw(struct heap* self, size_t size) {
  void* block = NULL;
  if (!util_atomic_add_if_less_uintptr(&self->bumpPointer, size, self->maxBumpPointer, (uintptr_t*) &block))
    return NULL;
  return block;
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

static size_t calcHeapBlockAlignment(size_t alignment) {
  return MAX(alignof(struct heap_block), alignment);
}

// Also compensate for alignment
// by giving extra space
static size_t calcHeapBlockSize(size_t alignment, size_t objectSize) {
  return ROUND_UP(sizeof(struct heap_block) + ROUND_UP(objectSize, alignment), alignof(struct heap_block));
}

static struct heap_block* adjustAlignmentAndInitDataPtr(struct heap_block* block, size_t alignment) {
  block = PTR_ALIGN(block, alignof(struct heap_block));
  block->dataPtr.ptr = PTR_ALIGN(&block->data, alignment);
  return block;
}

struct heap_block* heap_alloc_fast(struct heap* self, size_t dataAlignment, size_t objectSize) {
  struct local_heap_uwu* localHeap = getLocalHeap(self);
  struct heap_block* block = NULL;
  size_t size = calcHeapBlockSize(dataAlignment, objectSize);
  
  context_block_gc();
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    block = aligned_alloc(calcHeapBlockAlignment(dataAlignment), objectSize);
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
alloc_sucess:
  block = adjustAlignmentAndInitDataPtr(block, dataAlignment);
  initAllocatedBlock(self, block, dataAlignment, size, objectSize);
  if (postAllocHook(self, block) < 0) {
    heap_dealloc(self, block);
    block = NULL;
    goto alloc_failure;
  }
alloc_failure:
  context_unblock_gc();
  return block;
}

static void unlinkFromFreeList(struct heap* self, struct heap_block* block) {
  list_del(&block->node);
}

struct heap_block* heap_alloc(struct heap* self, size_t dataAlignment, size_t objectSize) {
  if (objectSize == 0)
    return NULL;
  struct heap_block* block = NULL;
  
  context_block_gc();
  if ((block = heap_alloc_fast(self, dataAlignment, objectSize)))
    return block;
  size_t blockSize = calcHeapBlockSize(dataAlignment, objectSize);
  
  // Slow alloc method
  mutex_lock(&self->lock);
  block = heap_find_free_block(self, blockSize);
  if (!block) 
    goto alloc_failed;
  
  block = splitFreeBlocks(self, block, blockSize);
  unlinkFromFreeList(self, block);
  
  block = adjustAlignmentAndInitDataPtr(block, dataAlignment);
  initAllocatedBlock(self, block, dataAlignment, blockSize, objectSize);
  
alloc_failed:
  mutex_unlock(&self->lock);
  
  if (block && postAllocHook(self, block) < 0) {
    heap_dealloc(self, block);
    block = NULL;
  }
  context_unblock_gc();
  return block;
}

void heap_clear(struct heap* self) {
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    return;
  
  mutex_lock(&self->lock);
  struct list_head* current = self->recentFreeBlocks.next;
  while (current != &self->recentAllocatedBlocks) {
    current = current->next;
    heap_dealloc(self, list_entry(current, struct heap_block, node));
  }
  
  // Resets allocation information
  list_head_init(&self->recentFreeBlocks);
  list_head_init(&self->recentAllocatedBlocks);
  atomic_store(&self->bumpPointer, 0);
  mutex_unlock(&self->lock);
}

int heap_is_belong_to_this_heap(struct heap* self, void* ptr) {
  return (uintptr_t) self->pool <= (uintptr_t) ptr &&
         (uintptr_t) self->pool <  (uintptr_t) (self->pool + self->size);
}
