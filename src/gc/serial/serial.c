#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "gc/gc.h"
#include "gc/gc_flags.h"
#include "gc/generic/generic.h"
#include "gc/serial/serial.h"
#include "util/util.h"

static void freeHook(struct gc_ops* self) {
  free(self);
}

struct gc_ops* gc_serial_new(gc_flags flags) {
  struct gc_ops* self = malloc(sizeof(*self));
  *self = (struct gc_ops) { GC_HOOKS_DEFAULT,
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

size_t gc_serial_preferred_promotion_size(gc_flags flags, int genID) {
  switch (genID) {
    case 0: return 2 * 1024;
    case 1: return 8 * 1024;
  }
  return SIZE_MAX;
}

int gc_serial_preferred_promotion_age(gc_flags flags, int genID) {
  switch (genID) {
    case 0: return 4;
    case 1: return 10;
  }
  return INT_MAX;
}
