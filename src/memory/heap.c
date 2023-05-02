#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <threads.h>

#include "context.h"
#include "gc/gc.h"
#include "heap.h"
#include "bug.h"
#include "config.h"
#include "heap_free_block_searchers.h"
#include "heap_local_heap.h"
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
  self->pool = ptr;
  self->destroyerUwU = destroyer;
  self->bumpPointer = (uintptr_t) ptr;
  self->maxBumpPointer = (uintptr_t) ptr + size;
  self->size = size;
  if (mutex_init(&self->lock) < 0)
    goto failure;
  
  list_head_init(&self->recentFreeBlocks);
  list_head_init(&self->recentAllocatedBlocks);
  return self;
failure:
  heap_free(self);
  return NULL;
}

void heap_free(struct heap* self) {
  mutex_cleanup(&self->lock);
  self->destroyerUwU(self->pool);
  free(self);
}

void heap_init(struct heap* self) {
  self->initialized = true;
}

int heap_param_set_local_heap_size(struct heap* self, size_t size) {
  self->localHeapSize = util_align_to_word(size);
  return 0;
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

static void onFreeHook(struct heap* self, struct heap_block* block) {
  gc_on_dealloc(&block->objMetadata);
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC))
    free(block->dataPtr.ptr);
  block->dataPtr = USERPTR_NULL;
}

static bool onAllocHook(struct heap* self, struct heap_block* block) {
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    block->dataPtr = USERPTR(malloc(block->objectSize));
    if (!block->dataPtr.ptr)
      return false;
  } else {
    block->dataPtr = USERPTR(&block->data);
  }
  
  gc_on_alloc(&block->objMetadata);
  return true;
}

// Try merge chain of free blocks
static void tryMergeFreeBlock(struct heap* self, struct heap_block* block) {
  BUG_ON(!block->isFree);
  
  struct list_head* current;
  list_for_each(current, &self->recentFreeBlocks) {
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
  list_for_each(current, &self->recentFreeBlocks)
    tryMergeFreeBlock(self, list_entry(current, struct heap_block, node));
  
  mutex_unlock(&self->lock);
}

void heap_dealloc(struct heap_block* block) {
  struct heap* self = context_current->heap;
  mutex_lock(&self->lock);
  
  // Double dealloc
  BUG_ON(block->isFree);
  
  onFreeHook(self, block);
  initFreeBlock(self, block, block->blockSize);
  
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    free(block->dataPtr.ptr);
    free(block);
    return;
  }
  
  // Add to free list
  list_add(&block->node, &self->recentFreeBlocks);
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

void heap_on_context_create(struct heap* self, struct context* context) {
  context->heap = self;
  context->localHeap = (struct heap_local_heap) {};
}

struct heap_block* heap_alloc_fast(size_t objectSize) {
  struct heap_local_heap* localHeap = &context_current->localHeap;
  struct heap* self = context_current->heap;
  struct heap_block* block = NULL;
  size_t size = util_align_to_word(sizeof(struct heap_block) + objectSize);
  
  context_block_gc();
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    block = malloc(objectSize);
    goto local_alloc_success;
  }
  
  // Skip allocation from local heap
  if (size > self->localHeapSize) {
    block = allocFastRaw(self, size);
    if (!block)
      goto alloc_failure;
    goto local_alloc_success;
  }
  
  // Get new local heap (cant fit)
  if (size > local_heap_get_size(localHeap)) {
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

static void unlinkFromFreeList(struct heap* self, struct heap_block* block) {
  list_del(&block->node);
}

struct heap_block* heap_alloc(size_t objectSize) {
  if (objectSize == 0)
    return NULL;
  struct heap_block* block = NULL;
  if (IS_ENABLED(CONFIG_HEAP_USE_MALLOC)) {
    block = malloc(objectSize);
    goto alloc_success;
  }
  
  block = heap_alloc_fast(objectSize);
  if (block)
    return block;
  size_t blockSize = util_align_to_word(sizeof(struct heap_block) + objectSize);
  
  // Slow alloc method
  struct heap* self = context_current->heap;
  mutex_lock(&self->lock);
  block = heap_find_free_block(self, blockSize);
  if (!block)
    return NULL;
  
  block = splitFreeBlocks(self, block, blockSize);
  unlinkFromFreeList(self, block);
  initAllocatedBlock(self, block, blockSize, objectSize);
  
  mutex_unlock(&self->lock);
  
alloc_success:
  context_block_gc();
  if (!onAllocHook(self, block)) {
    heap_dealloc(block);
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
    heap_dealloc(list_entry(current, struct heap_block, node));
  }
  
  // Resets allocation information
  list_head_init(&self->recentFreeBlocks);
  list_head_init(&self->recentAllocatedBlocks);
  atomic_store(&self->bumpPointer, 0);
  mutex_unlock(&self->lock);
}
