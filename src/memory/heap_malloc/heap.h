#ifndef header_1703585559_e175c7e5_643d_4273_87b2_1e5433748652_heap_h
#define header_1703585559_e175c7e5_643d_4273_87b2_1e5433748652_heap_h

// IWYU pragma: private, include "memory/heap.h"

#include "concurrency/mutex.h"
#include "object/object.h"
#include "util/list_head.h"

#include "../heap_common_types.h"

struct heap {
  struct mutex lock;
  
  size_t usage;
  size_t totalSize; 

  // Purpose of GC traversing whole heap
  struct list_head allocatedBlocks;
};

struct heap_block {
  struct list_head node;
  size_t blockSize;
  void* allocPtr;
  struct object objMetadata;
};

void* heap_block_get_ptr(struct heap_block* block);
struct heap_block* heap_alloc(struct heap* self, size_t size);
void heap_dealloc(struct heap* self, struct heap_block* block);
void heap_clear(struct heap* self);

struct heap* heap_new(const struct heap_init_params* params);
void heap_free(struct heap* self);

// Move src to dest while leaving
// src at undefined state
void heap_move(struct heap_block* src, struct heap_block* dest);

#endif

