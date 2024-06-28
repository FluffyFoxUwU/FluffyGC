#ifndef UWU_F514FBEE_E733_4F66_9DE9_E71E5AC33DCD_UWU
#define UWU_F514FBEE_E733_4F66_9DE9_E71E5AC33DCD_UWU

#include <stddef.h>

#include "gc/gc.h"
#include "memory/arena.h"

struct generation {
  struct heap* ownerHeap;
  struct alloc_tracker* allocTracker;
  struct gc_per_generation_state* gcState;
};

struct generation* generation_new(size_t size);
void generation_free(struct generation* self);

struct alloc_unit* generation_alloc(struct generation* self, size_t size);

#endif
