#include <stdlib.h>

#include <flup/data_structs/buffer/circular_buffer.h>

#include "memory/arena.h"
#include "heap/generation.h"

#include "gc.h"

void gc_on_allocate(struct arena_block* block, struct generation* gen) {
  block->gcMetadata = (struct gc_block_metadata) {
    .markBit = gen->gcState->flipColor ? 1 : 0
  };
}

struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen) {
  struct gc_per_generation_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_per_generation_state) {
    .ownerGen = gen
  };
  
  if (!(self->markQueue = flup_circular_buffer_new(GC_MARK_QUEUE_SIZE)))
    goto failure;
  return self;

failure:
  gc_per_generation_state_free(self);
  return NULL;
}

void gc_per_generation_state_free(struct gc_per_generation_state* self) {
  if (!self)
    return;
  
  flup_circular_buffer_free(self->markQueue);
  free(self);
}

