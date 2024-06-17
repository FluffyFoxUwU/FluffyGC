#include <stddef.h>
#include <stdlib.h>

#include "generation.h"
#include "gc/gc.h"
#include "heap/heap.h"
#include "memory/arena.h"

struct generation* generation_new(size_t sz) {
  struct generation* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct generation) {};
  if (!(self->arena = arena_new(sz)))
    goto failure;
  
  if (!(self->gcState = gc_per_generation_state_new(self)))
    goto failure;
  return self;

failure:
  generation_free(self);
  return NULL;
}

void generation_free(struct generation* self) {
  if (!self)
    return;
  gc_per_generation_state_free(self->gcState);
  arena_free(self->arena);
  free(self);
}

struct arena_block* generation_alloc(struct generation* self, size_t size) {
  struct arena_block* block = arena_alloc(self->arena, size);
  if (!block)
    return NULL;
  
  return block;
}

