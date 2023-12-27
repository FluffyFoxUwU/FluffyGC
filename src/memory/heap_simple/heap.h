#ifndef _headers_1674289012_FluffyGC_heap
#define _headers_1674289012_FluffyGC_heap

// IWYU pragma: private, include "memory/heap.h"

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

#include "../heap_common_types.h"

// This heap can do both bump pointer and
// free lists method. This is the lowest
// layer which primarily dealing with allocation

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
  size_t blockSize;
  
  struct object objMetadata;

  alignas(max_align_t) char data;
};


/*
Allocation ordered by frequency (cascades to next if doesnt fit)

1. Local heap bump pointer
2. Young bump pointer
3. Old bump pointer 
4. Old free list based
*/

void* heap_block_get_ptr(struct heap_block* block);
struct heap_block* heap_alloc(struct heap* self, size_t size);
void heap_dealloc(struct heap* self, struct heap_block* block);

// Nukes everything
void heap_clear(struct heap* self);

struct heap* heap_new(const struct heap_init_params* params);
void heap_free(struct heap* self);

// Move src to dest while leaving
// src at undefined state
void heap_move(struct heap_block* src, struct heap_block* dest);

#endif

