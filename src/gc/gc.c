#include <stdlib.h>

#include "gc.h"
#include "gc/nop/nop.h"
#include "bug.h"

thread_local struct gc_struct* gc_current = NULL;

struct gc_struct* gc_new(enum gc_algorithm algo, int gcFlags) {
  struct gc_struct* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  switch (algo) {
    case GC_UNKNOWN:
      BUG();
    case GC_NOP_GC:
      self->hooks = gc_nop_new(gcFlags);
      break;
  }
  
  if (!self->hooks)
    goto failure;
  
  return self;
failure:
  gc_free(self);
  return NULL;
}

void gc_free(struct gc_struct* self) {
  self->hooks->free(self->hooks);
  free(self);
}
