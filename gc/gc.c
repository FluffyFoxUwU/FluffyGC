#include <stdatomic.h>
#include <stdlib.h>

#include <flup/data_structs/buffer.h>

#include "memory/arena.h"
#include "heap/generation.h"

#include "gc.h"

void gc_on_allocate(struct arena_block* block, struct generation* gen) {
  block->gcMetadata = (struct gc_block_metadata) {
    .markBit = !gen->gcState->mutatorMarkedBitValue
  };
}

void gc_on_write(struct arena_block* objectWhichIsGoingToBeOverwritten) {
  struct gc_block_metadata* metadata = &objectWhichIsGoingToBeOverwritten->gcMetadata;
  struct gc_per_generation_state* gcState = metadata->owningGeneration->gcState;
  
  bool prevMarkBit = atomic_exchange(&metadata->markBit, gcState->GCMarkedBitValue);
  if (prevMarkBit == gcState->GCMarkedBitValue)
    return;
  
  // Enqueue an pointer
  flup_buffer_write_no_fail(gcState->mutatorMarkQueue, &objectWhichIsGoingToBeOverwritten, sizeof(void*));
}

struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen) {
  struct gc_per_generation_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_per_generation_state) {
    .ownerGen = gen
  };
  
  if (!(self->mutatorMarkQueue = flup_buffer_new(GC_MARK_QUEUE_SIZE)))
    goto failure;
  return self;

failure:
  gc_per_generation_state_free(self);
  return NULL;
}

void gc_per_generation_state_free(struct gc_per_generation_state* self) {
  if (!self)
    return;
  
  flup_buffer_free(self->mutatorMarkQueue);
  free(self);
}

