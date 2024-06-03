#include <stddef.h>
#include <stdlib.h>

#include "generation.h"
#include "memory/arena.h"
#include "util/bitmap.h"

struct generation* generation_new(size_t sz) {
  struct generation* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct generation) {};
  if (!(self->arena = arena_new(sz)))
    goto failure;
  
  if (!(self->crossGenerationReferenceMap = bitmap_new(self->arena->maxObjectCount)))
    goto failure;
  return self;

failure:
  generation_free(self);
  return NULL;
}

void generation_free(struct generation* self) {
  if (!self)
    return;
  arena_free(self->arena);
  bitmap_free(self->crossGenerationReferenceMap);
  free(self);
}


