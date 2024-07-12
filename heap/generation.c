#include <stddef.h>
#include <stdlib.h>

#include "generation.h"
#include "gc/gc.h"
#include "memory/alloc_tracker.h"
#include "heap/heap.h"

struct generation* generation_new(size_t sz) {
  struct generation* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct generation) {};
  if (!(self->allocTracker = alloc_tracker_new(sz)))
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
  gc_perform_shutdown(self->gcState);
  gc_per_generation_state_free(self->gcState);
  alloc_tracker_free(self->allocTracker);
  free(self);
}

struct alloc_unit* generation_alloc(struct generation* self, size_t size) {
  struct alloc_unit* block = alloc_tracker_alloc(self->allocTracker, heap_get_alloc_context(self->ownerHeap), size);
  if (!block)
    return NULL;
  
  return block;
}

