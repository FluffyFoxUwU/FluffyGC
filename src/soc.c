#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bitops.h"
#include "config.h"
#include "soc.h"
#include "bug.h"
#include "util.h"
#include "vec.h"

static void freeChunk(struct soc_chunk* self) {
  if (!self)
    return;
  
  free(self->pool);
  free(self);
}

static void* chunkAllocObject(struct soc_chunk* self) {
  struct soc_free_node* obj = self->firstFreeNode;
  if (!obj)
    goto failure;
  
  self->firstFreeNode = obj->next;
  if (!self->firstFreeNode)
    clearbit(self->id, self->owner->partialChunksBitmap);
failure:
  return obj;
}

static struct soc_chunk* getOwningChunk(void* ptr) {
  return *(void**) (ptr - ((uintptr_t) ptr % SOC_CHUNK_ALIGNMENT));
}

static void chunkDeallocObject(void* ptr) {
  struct soc_chunk* self = getOwningChunk(ptr);
  struct soc_free_node* freeNode = ptr;
  freeNode->next = self->firstFreeNode;
  self->firstFreeNode = freeNode;
  setbit(self->id, self->owner->partialChunksBitmap);
}

static struct soc_chunk* newChunk(struct small_object_cache* owner) {
  struct soc_chunk* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct soc_chunk) {
    .owner = owner,
    .pool = aligned_alloc(SOC_CHUNK_ALIGNMENT, SOC_CHUNK_SIZE)
  };
  
  self->totalObjectsCount = UTIL_DIV_ROUND_DOWN(SOC_CHUNK_SIZE, owner->objectSize);
  self->pool = self->pool;
  if (!self->pool)
    goto failure;
  
  // Init free lists
  struct soc_free_node* current = self->pool;
  for (int i = 1; i < self->totalObjectsCount; i++) {
    current->next = self->pool + owner->objectSize * i;
    current = current->next;
  }
  self->firstFreeNode = self->pool;
  
  // The last node point to outside of the pool
  (current - owner->objectSize)->next = NULL;
  
  // Always reserve first object to store pointer to owning chunk
  *((void**) chunkAllocObject(self)) = self;
  return self;

failure:
  freeChunk(self);
  return NULL;
}

static struct soc_chunk* extendCache(struct small_object_cache* self) {
  struct soc_chunk* chunk = newChunk(self);
  if (!chunk)
    return NULL;
  
  int oldCapacity = self->chunks.capacity;
  if (vec_push(&self->chunks, chunk) < 0) 
    goto failure;
  
  if (oldCapacity != self->chunks.capacity) {
    int oldCapacityWords = BITS_TO_LONGS(oldCapacity) * sizeof(unsigned long);
    int newCapacityWords = BITS_TO_LONGS(self->chunks.capacity) * sizeof(unsigned long);
    unsigned long* newBitmap = util_realloc(self->partialChunksBitmap, oldCapacityWords, newCapacityWords, UTIL_MEM_ZERO);
    
    if (!newBitmap)
      goto failure;
    
    self->partialChunksBitmap = newBitmap;
  }
  
  chunk->id = self->chunks.length - 1;
  setbit(chunk->id, self->partialChunksBitmap);
  return chunk;

failure:
  freeChunk(chunk);
  return NULL;
}

static struct soc_chunk* findPartialChunk(struct small_object_cache* self) {
  if (!self->partialChunksBitmap)
    return NULL;
  
  int partialChunkLoc = findbit(true, self->partialChunksBitmap, self->chunks.capacity);
  if (partialChunkLoc < 0)
    return NULL;
  
  return self->chunks.data[partialChunkLoc];
}

static struct soc_chunk* getChunk(struct small_object_cache* self) {
  struct soc_chunk* chunk = findPartialChunk(self);
  if (chunk)
    return chunk;
  return extendCache(self);
}

struct small_object_cache* soc_new(size_t objectSize, int reservedChunks) {
  BUG_ON(reservedChunks < 0);
  if (objectSize > SOC_MAX_OBJECT_SIZE) {
    WARN();
    return NULL;
  }
  objectSize = MAX(objectSize, SOC_MIN_OBJECT_SIZE);
  
  struct small_object_cache* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct small_object_cache) {
    .objectSize = objectSize
  };
  vec_init(&self->chunks);
  
  for (int i = 0; i < reservedChunks; i++) {
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
  
  for (unsigned int i = 0; i < self->chunks.length; i++)
    freeChunk(self->chunks.data[i]);
  
  vec_deinit(&self->chunks);
  free(self->partialChunksBitmap);
  free(self);
}

void* soc_alloc(struct small_object_cache* self) {
  if (IS_ENABLED(SOC_USE_MALLOC))
    return malloc(self->objectSize);
  
  struct soc_chunk* chunk = getChunk(self);
  if (!chunk)
    return NULL;
  return chunkAllocObject(chunk);
}

void soc_dealloc(struct small_object_cache* self, void* ptr) {
  if (IS_ENABLED(SOC_USE_MALLOC))
    return free(ptr);
  
  if (!ptr)
    return;
  chunkDeallocObject(ptr);
}
