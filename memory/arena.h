#ifndef UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU
#define UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU

#include <stdatomic.h>
#include <stddef.h>

#include <flup/data_structs/list_head.h>
#include <flup/concurrency/mutex.h>

#include "gc/gc.h"
#include "object/descriptor.h"

struct arena {
  atomic_size_t currentUsage;
  size_t maxSize;
  
  _Atomic(struct arena_block*) head;
};

struct arena_block {
  // If current->next == NULL then its end of detached head :3
  struct arena_block* next;
  size_t size;
  struct descriptor* desc;
  struct gc_block_metadata gcMetadata;
  void* data;
};

struct arena_block* arena_detach_head(struct arena* self);
bool arena_is_end_of_detached_head(struct arena_block* blk);
void arena_move_one_block_from_detached_to_real_head(struct arena* self, struct arena_block* blk);

struct arena* arena_new(size_t size);
void arena_free(struct arena* self);

struct arena_block* arena_alloc(struct arena* self, size_t size);
void arena_dealloc(struct arena* self, struct arena_block* blk);

void arena_wipe(struct arena* self);

#endif
