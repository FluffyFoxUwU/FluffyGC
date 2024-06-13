#ifndef UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU
#define UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU

#include <stddef.h>

#include <flup/data_structs/dyn_array.h>
#include <flup/data_structs/list_head.h>
#include <flup/concurrency/mutex.h>

#include "gc/gc.h"

struct arena {
  flup_mutex* lock;
  
  size_t currentUsage;
  size_t maxSize;
  size_t objectCount;
  
  flup_dyn_array* blocks;
  flup_list_head freeList;
};

struct arena_block {
  bool used;
  struct gc_block_metadata gcMetadata;
  flup_list_head node;
  void* data;
};

struct arena* arena_new(size_t size);
void arena_free(struct arena* self);

struct arena_block* arena_alloc(struct arena* self, size_t size);
void arena_dealloc(struct arena* self, struct arena_block* blk);

void arena_wipe(struct arena* self);

#endif
