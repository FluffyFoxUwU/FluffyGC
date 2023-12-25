#ifndef _headers_1674289012_FluffyGC_heap
#define _headers_1674289012_FluffyGC_heap

#include <pthread.h>
#include <stdalign.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <threads.h>

#include "object/object.h"
#include "concurrency/thread_local.h"
#include "concurrency/mutex.h"
#include "util/list_head.h"

// This heap can do both bump pointer and
// free lists method. This is the lowest
// layer which primarily dealing with allocation

struct context;

struct heap_init_params {
  size_t localHeapSize;
  size_t size;
};

struct heap {
  struct mutex lock;
  struct thread_local_struct localHeap;
  
  void* base;
  size_t localHeapSize;
  
  atomic_uintptr_t bumpPointer;
  uintptr_t maxBumpPointer;
  
  atomic_size_t usage;
  size_t size;
  
  // Storing list of freed blocks
  // with head being the most recently
  // free'd
  struct list_head recentFreeBlocks;
  
  // Purpose of GC traversing whole heap
  struct list_head allocatedBlocks;
};

struct heap_block {
  struct list_head node;
  
  // Including this structure as well
  size_t blockSize;
  size_t dataSize;
  
  struct object objMetadata;
  
  union {
    alignas(max_align_t) char data;
    // Only if CONFIG_HEAP_USE_MALLOC enabled
    void* allocPtr;
  };
};

/*
Plan for compacting heap_block structure
1. Nuke alignment and assume max_align_t

struct  heap_block alignas(max_align_t) {
  struct list_head node;
  size_t blockSize;
  bool isFree;

  struct object objMetadata;
  char data[];
};

 */

void* heap_block_get_ptr(struct heap_block* block);

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

struct heap* heap_new(const struct heap_init_params* params);
void heap_free(struct heap* self);

// Parameters
// Pass size == 0 to disable local heap
int heap_param_set_local_heap_size(struct heap* self, size_t size);

#endif

