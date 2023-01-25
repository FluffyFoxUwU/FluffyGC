#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>

#include "context.h"
#include "heap.h"
#include "bug.h"
#include "config.h"
#include "heap_free_block_searchers.h"
#include "heap_local_heap.h"
#include "free_list_sorter.h"
#include "util.h"

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
  self->pool = ptr;
  self->destroyerUwU = destroyer;
  self->bumpPointer = (uintptr_t) ptr;
  self->maxBumpPointer = (uintptr_t) ptr + size;
  if (pthread_rwlock_init(&self->lock, NULL) != 0)
    goto failure;
  self->lockInited = true;
  
  return self;
failure:
  heap_free(self);
  return NULL;
}

void heap_free(struct heap* self) {
  int ret = 0;
  if (self->lockInited)
    ret = pthread_rwlock_destroy(&self->lock);
  BUG_ON(ret != 0);
  
  self->destroyerUwU(self->pool);
  free(self);
}

void heap_init(struct heap* self) {
  self->initialized = true;
}

void heap_param_set_local_heap_size(struct heap* self, size_t size) {
  self->localHeapSize = size;
}

static void initAllocatedBlock(struct heap* self, struct heap_block* block, size_t blockSize, size_t objectSize) {
  *block = (struct heap_block) {
    .isFree = false,
    .blockSize = blockSize,
    .objectSize = objectSize
  };
}

static void initFreeBlock(struct heap* self, struct heap_block* block, size_t blockSize) {
  *block = (struct heap_block) {
    .isFree = true,
    .blockSize = blockSize
  };
}

static void unlinkFromFreeList(struct heap* self, struct heap_block* block) {
  if (block->prev)
    block->prev->next = block->next;
  if (self->recentFreeBlocks == block)
    self->recentFreeBlocks = block->next;
  if (block->next)
    block->next->prev = block->prev;
}

static void onFreeHook(struct heap* self, struct heap_block* block) {
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    free(block->dataPtr);
  block->dataPtr = NULL;
}

static bool onAllocHook(struct heap* self, struct heap_block* block) {
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    if (!(block->dataPtr = malloc(block->objectSize)))
      return false;
  } else {
    block->dataPtr = &block->data;
  }
  
  return true;
}

// Try merge chain of free blocks
static void tryMergeFreeBlock(struct heap* self, struct heap_block* block) {
  BUG_ON(!block->isFree);
  
  struct heap_block* current = block->next;
  while (current) {
    BUG_ON(!current->isFree);
    // Free block arent next current block anymore
    if (((void*) block) + block->blockSize != current)
      break;
    
    // Merge two blocks
    block->next = current->next;
    if (current->next)
      current->next->prev = block;
    block->blockSize += current->blockSize;
    
    current = current->next;
  }
}

void heap_merge_free_blocks(struct heap* self) {
  pthread_rwlock_wrlock(&self->lock);
  heap_free_list_sort(self);
  
  struct heap_block* block = self->recentFreeBlocks;
  
  while (block) {
    tryMergeFreeBlock(self, block);
    block = block->next;
  }
  pthread_rwlock_unlock(&self->lock);
}

void heap_dealloc(struct heap_block* block) {
  struct heap* self = context_current->heap;
  pthread_rwlock_wrlock(&self->lock);
  
  // Double free
  BUG_ON(block->isFree);
  
  onFreeHook(self, block);
  initFreeBlock(self, block, block->blockSize);
  
  // Add to free list
  block->prev = NULL;
  block->next = self->recentFreeBlocks;
  if (block->next)
    block->next->prev = block;
  
  self->recentFreeBlocks = block;
  pthread_rwlock_unlock(&self->lock);
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
  if (block->blockSize - size < sizeof(struct heap_block))
    return block;
  
  struct heap_block* blockA = block;
  struct heap_block* blockB = ((void*) block) + size;
  initFreeBlock(self, blockB, block->blockSize - size);
  
  // TODO: Comment these four operations
  blockB->next = blockA->next;
  blockA->next = blockB;
  
  blockB->prev = blockA;
  if (blockB->next)
    blockB->next->prev = blockB;
  return block;
}

void heap_on_thread_create(struct heap* self, struct context* thread) {
  thread->heap = self;
  thread->localHeap = (struct heap_local_heap) {};
}

struct heap_block* heap_alloc_fast(size_t objectSize) {
  struct heap_local_heap* localHeap = &context_current->localHeap;
  struct heap* self = context_current->heap;
  struct heap_block* block = NULL;
  size_t size = util_align_to_word(sizeof(struct heap_block) + objectSize);
  
  context_block_gc();
  if (size > self->localHeapSize) {
    block = allocFastRaw(self, size);
    if (!block)
      goto alloc_failure;
    goto local_alloc_success;
  }
  
  // Get new local heap (cant fit)
  if (size > localHeap->endLocalHeap - localHeap->bumpPointer) {
    uintptr_t newLocalHeap = (uintptr_t) allocFastRaw(self, self->localHeapSize);
    if (!newLocalHeap)
      goto alloc_failure;
    localHeap->bumpPointer = newLocalHeap;
    localHeap->endLocalHeap = newLocalHeap + self->localHeapSize;
  }

  block = (struct heap_block*) localHeap->bumpPointer;
  localHeap->bumpPointer += size;
local_alloc_success:
  initAllocatedBlock(self, block, size, objectSize);
  if (!onAllocHook(self, block)) {
    heap_dealloc(block);
    block = NULL;
    goto alloc_failure;
  }
alloc_failure:
  context_unblock_gc();
  return block;
}

struct heap_block* heap_alloc(size_t objectSize) {
  if (objectSize == 0)
    return NULL;
  
  struct heap_block* block = heap_alloc_fast(objectSize);
  if (block)
    return block;
  size_t size = util_align_to_word(sizeof(struct heap_block) + objectSize);
  
  // Slow alloc method
  struct heap* self = context_current->heap;
  pthread_rwlock_wrlock(&self->lock);
  context_block_gc();
  
  block = heap_find_free_block(self, self->recentFreeBlocks, size);
  if (!block)
    goto fail_alloc;
  
  block = splitFreeBlocks(self, block, size);
  unlinkFromFreeList(self, block);
  initAllocatedBlock(self, block, size, objectSize);
fail_alloc:
  pthread_rwlock_unlock(&self->lock);
  
  if (block && !onAllocHook(self, block)) {
    heap_dealloc(block);
    block = NULL;
  }
  
  context_unblock_gc();
  return block;
}

void heap_clear(struct heap* self) {
  pthread_rwlock_wrlock(&self->lock);
  self->recentFreeBlocks = NULL;
  atomic_store(&self->bumpPointer, 0);
  pthread_rwlock_unlock(&self->lock);
}
