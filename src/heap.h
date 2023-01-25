#ifndef _headers_1674289012_FluffyGC_heap
#define _headers_1674289012_FluffyGC_heap

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#include "object.h"

// This heap can do both bump pointer and
// free lists method.

struct context;

struct heap {
  pthread_rwlock_t lock;
  bool lockInited;
  
  void* pool;
  void (*destroyerUwU)(void*);
  
  bool initialized;
  size_t localHeapSize;
  struct heap_block* recentFreeBlocks;
  
  atomic_uintptr_t bumpPointer;
  uintptr_t maxBumpPointer;
};

struct heap_block {
  struct heap_block* next;
  struct heap_block* prev;
  
  bool isFree;
  // Including this structure as well
  size_t blockSize;
  size_t objectSize;
  
  struct object objMetadata;
  void* dataPtr;
  char data[];
};

// Each thread have its own local heap
// Which allocated from larger global heap
// Aiming to reduce global lock contention
#include "heap_local_heap.h"

void heap_merge_free_blocks(struct heap* self);

/*
Allocation ordered by frequency (cascades to next if doesnt fit)

1. Local heap bump pointer
2. Young bump pointer
3. Old bump pointer 
4. Old free list based
*/

struct heap_block* heap_alloc_fast(size_t size);
struct heap_block* heap_alloc(size_t size);
void heap_dealloc(struct heap_block* block);

// This is thread unsafe
void heap_clear(struct heap* self);

struct heap* heap_new(size_t size);
struct heap* heap_from_existing(size_t size, void* ptr, void (*destroyer)(void*));
void heap_free(struct heap* self);

void heap_on_thread_create(struct heap* self, struct context* thread);

// Parameter cant be change anymore after this call
void heap_init(struct heap* self);

// Parameters
// Pass size == 0 to disable local heap
void heap_param_set_local_heap_size(struct heap* self, size_t size);

#endif

