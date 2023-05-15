#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bitops.h"
#include "config.h"
#include "soc.h"
#include "bug.h"
#include "util/list_head.h"
#include "util/util.h"
#include "vec.h"

static void freeChunk(struct soc_chunk* self) {
  if (!self)
    return;
  
  free(self->pool);
  free(self);
}

static void* chunkAllocObject(struct soc_chunk* self) {
  struct soc_free_node* obj = self->firstFreeNode;
  BUG_ON(!obj);
  
  self->firstFreeNode = obj->next;
  
  // Remove from partial chunk to full chunk list if full
  if (self->firstFreeNode == NULL) {
    list_del(&self->list);
    list_add(&self->list, &self->owner->fullChunksList);
  }
  return obj;
}

// TODO: Better way than this
static struct soc_chunk* getOwningChunk(void* ptr) {
  BUG();
  return *(void**) (ptr - ((uintptr_t) ptr % 1));
}

void* soc_alloc(struct small_object_cache* self) {
  return soc_alloc_explicit(self, NULL);
}

void soc_dealloc(struct small_object_cache* self, void* ptr) {
  soc_dealloc_explicit(self, getOwningChunk(ptr), ptr);
}

static void chunkDeallocObject(struct soc_chunk* self, void* ptr) {
  // Move from full chunk to partial chunk list if this was full
  if (self->firstFreeNode == NULL) {
    list_del(&self->list);
    list_add_tail(&self->list, &self->owner->partialChunksList);
  }
  
  struct soc_free_node* freeNode = ptr;
  freeNode->next = self->firstFreeNode;
  self->firstFreeNode = freeNode;
}

static struct soc_chunk* newChunk(struct small_object_cache* owner) {
  struct soc_chunk* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct soc_chunk) {
    .owner = owner,
    .pool = aligned_alloc(owner->alignment, owner->chunkSize)
  };
  
  self->totalObjectsCount = DIV_ROUND_DOWN(owner->chunkSize, owner->objectSize);
  if (!self->pool)
    goto failure;
  
  // Init free lists
  struct soc_free_node* prev = NULL;
  struct soc_free_node* current = self->pool;
  for (int i = 0; i < self->totalObjectsCount; i++) {
    // NOTE: no PTR_ALIGN because objectSize already alignment adjusted
    // so multiple of objectSize statisfies the requirement
    current->next = self->pool + owner->objectSize * i;
    prev = current;
    current = current->next;
  }
  self->firstFreeNode = self->pool;
  
  // The last node point to outside of the pool
  if (prev)
    prev->next = NULL;
  return self;

failure:
  freeChunk(self);
  return NULL;
}

static struct soc_chunk* extendCache(struct small_object_cache* self) {
  struct soc_chunk* chunk = newChunk(self);
  if (!chunk)
    return NULL;
  
  list_add(&chunk->list, &self->partialChunksList);
  return chunk;
}

static struct soc_chunk* findPartialChunk(struct small_object_cache* self) {
  if (list_is_empty(&self->partialChunksList))
    return NULL;
  return list_first_entry(&self->partialChunksList, struct soc_chunk, list);
}

static struct soc_chunk* getChunk(struct small_object_cache* self) {
  struct soc_chunk* chunk = findPartialChunk(self);
  if (chunk)
    return chunk;
  return extendCache(self);
}

struct small_object_cache* soc_new(size_t alignment, size_t objectSize, int preallocCount) {
  return soc_new_with_chunk_size(SOC_DEFAULT_CHUNK_SIZE, alignment, objectSize, preallocCount);
}

struct small_object_cache* soc_new_with_chunk_size(size_t chunkSize, size_t alignment, size_t objectSize, int preallocCount) {
  alignment = MAX(alignof(struct soc_free_node), alignment);
  
  // If using system malloc ignore the size adjustment
  // leave it as it is for the system malloc to handle
  if (!IS_ENABLED(CONFIG_SOC_USE_MALLOC)) {
    objectSize = ROUND_UP(MAX(objectSize, SOC_MIN_OBJECT_SIZE), alignment);
    chunkSize = ROUND_UP(chunkSize, objectSize);
  }
  
  BUG_ON(preallocCount < 0);
  
  if (IS_ENABLED(CONFIG_FUZZ_SOC)) {
    if (chunkSize / objectSize < SOC_MIN_OBJECT_COUNT)
      chunkSize = objectSize * SOC_MIN_OBJECT_COUNT;
  } else {
    BUG_ON(chunkSize / objectSize < SOC_MIN_OBJECT_COUNT);
  }
  struct small_object_cache* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct small_object_cache) {
    .objectSize = objectSize,
    .alignment = alignment,
    .chunkSize = chunkSize
  };
  
  list_head_init(&self->fullChunksList);
  list_head_init(&self->partialChunksList);
  
  if (IS_ENABLED(CONFIG_SOC_USE_MALLOC))
    return self;
  
  for (int i = 0; i < preallocCount; i++) {
    struct soc_chunk* chunk = extendCache(self);
    if (!chunk)
      goto failure;
    chunk->isReserved = true;
  }
  
  return self;

failure:
  soc_free(self);
  return NULL;
}

void soc_free(struct small_object_cache* self) {
  if (!self)
    return;
  
  // The static one (defined using SOC_DEFINE)
  // dont need to be cleaned
  if (self->isStatic) {
    BUG();
    return;
  }
  
  if (IS_ENABLED(CONFIG_SOC_USE_MALLOC)) {
    free(self);
    return;
  }
  
  struct list_head* current;
  struct list_head* n;
  list_for_each_safe(current, n, &self->fullChunksList)
    freeChunk(list_entry(current, struct soc_chunk, list));
  
  n = NULL;
  current = NULL;
  list_for_each_safe(current, n, &self->partialChunksList)
    freeChunk(list_entry(current, struct soc_chunk, list));

  free(self);
}

void* soc_alloc_explicit(struct small_object_cache* self, struct soc_chunk** chunkPtr) {
  if (IS_ENABLED(CONFIG_SOC_USE_MALLOC))
    return aligned_alloc(self->alignment, self->objectSize);
  
  struct soc_chunk* chunk = getChunk(self);
  void* res = NULL;
  if (chunk)
    res = chunkAllocObject(chunk);
  
  if (chunkPtr)
    *chunkPtr = chunk;
  return res;
}

void soc_dealloc_explicit(struct small_object_cache* self, struct soc_chunk* chunk, void* ptr) {
  if (IS_ENABLED(CONFIG_SOC_USE_MALLOC))
    return free(ptr);
  
  if (!ptr)
    return;
  chunkDeallocObject(chunk, ptr);
}
