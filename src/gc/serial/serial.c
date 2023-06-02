#include <stdlib.h>

#include "gc/gc.h"
#include "gc/gc_flags.h"
#include "gc/generic/generic.h"
#include "gc/nop/nop.h"
#include "util/util.h"

static void freeHook(struct gc_hooks* self) {
  free(self);
}

struct gc_hooks* gc_serial_new(gc_flags flags) {
  struct gc_hooks* self = malloc(sizeof(*self));
  *self = (struct gc_hooks) { GC_HOOKS_DEFAULT,
    .collect = gc_generic_collect
  };
  self->free = freeHook;
  return self;
}

int gc_serial_generation_count(gc_flags flags) {
  if (flags & SERIAL_GC_USE_3_GENERATIONS)
    return 3;
  else if (flags & SERIAL_GC_USE_2_GENERATIONS)
    return 2;
  return 1;
}

bool gc_serial_use_fast_on_gen(gc_flags flags, int genID) {
  return true;
}
