#ifndef _headers_1674289012_FluffyGC_heap
#define _headers_1674289012_FluffyGC_heap

#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#include "concurrency/thread_local.h"
#include "object/object.h"
#include "concurrency/mutex.h"
#include "util/list_head.h"

// This heap can do both bump pointer and
// free lists method. This is the lowest
// layer which primarily dealing with allocation

struct context;

struct heap {
  struct mutex lock;
  struct thread_local_struct localHeapKey;
  
  void* pool;
  void (*destroyerUwU)(void*);
  
  bool initialized;
  size_t localHeapSize;
  
  atomic_uintptr_t bumpPointer;
  uintptr_t maxBumpPointer;
  size_t size;
  
  struct list_head recentFreeBlocks;
  struct list_head recentAllocatedBlocks;
};

struct heap_block {
  struct list_head node;
  
  bool isFree;
  
  // Including this structure as well
  size_t blockSize;
  
  // The size of `data`
  size_t dataSize;
  
  struct object objMetadata;
  struct userptr dataPtr;
  char data[];
};

// Each thread have its own local heap
// Which allocated from larger global heap
// Aiming to reduce global lock contention

void heap_merge_free_blocks(struct heap* self);

/*
Allocation ordered by frequency (cascades to next if doesnt fit)

1. Local heap bump pointer
2. Young bump pointer
3. Old bump pointer 
4. Old free list based
*/

struct heap_block* heap_alloc_fast(struct heap* self, size_t size);
struct heap_block* heap_alloc(struct heap* self, size_t size);
void heap_dealloc(struct heap* self, struct heap_block* block);

// This is thread unsafe in a sense it nukes
// *EVERYTHING* no matter if another accessing it
void heap_clear(struct heap* self);

struct heap* heap_new(size_t size);
struct heap* heap_from_existing(size_t size, void* ptr, void (*destroyer)(void*));
void heap_free(struct heap* self);

// Parameter cant be change anymore after this call
void heap_init(struct heap* self);

// Parameters
// Pass size == 0 to disable local heap
int heap_param_set_local_heap_size(struct heap* self, size_t size);

int heap_is_belong_to_this_heap(struct heap* self, void* ptr);

#endif

