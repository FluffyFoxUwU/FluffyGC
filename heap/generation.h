#ifndef UWU_F514FBEE_E733_4F66_9DE9_E71E5AC33DCD_UWU
#define UWU_F514FBEE_E733_4F66_9DE9_E71E5AC33DCD_UWU

#include <stddef.h>

#include "memory/arena.h"
#include "util/bitmap.h"

struct generation {
  struct arena* arena;
  // Bit map of arena block which has
  // direct reference to outside of current
  // generation (the bit offset is the block's
  // index in the arena array)
  struct bitmap* crossGenerationReferenceMap;
};

struct generation* generation_new(size_t sz);
void generation_free(struct generation* self);

#endif
