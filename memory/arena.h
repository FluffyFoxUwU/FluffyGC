#ifndef UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU
#define UWU_F15ECD4B_1EE0_482D_8E14_3B523A879538_UWU

#include <stddef.h>

#include <flup/data_structs/dyn_array.h>
#include <flup/concurrency/mutex.h>

struct arena {
  flup_mutex* lock;
  
  size_t currentUsage;
  size_t maxSize;
  
  flup_dyn_array* blocks;
};

struct arena_block {
  size_t size;
  void* data;
};

struct arena* arena_new(size_t size);
void arena_free(struct arena* self);

struct arena_block* arena_alloc(struct arena* self, size_t size);
void arena_wipe(struct arena* self);

#endif
