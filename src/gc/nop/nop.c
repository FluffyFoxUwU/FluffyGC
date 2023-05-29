#include <stdlib.h>

#include "gc/gc.h"
#include "gc/nop/nop.h"
#include "util/util.h"

static void freeHook(struct gc_hooks* self) {
  free(self);
}

static int mark(struct generation* gen) {
  return 0;
}

static size_t collect(struct generation*, size_t sizeHint) {
  return 0;
}

static size_t compact(struct generation*, size_t sizeHint) {
  return 0;
}

struct gc_hooks* gc_nop_new(int flags) {
  struct gc_hooks* self = malloc(sizeof(*self));
  *self = (struct gc_hooks) { GC_HOOKS_DEFAULT,
    .mark = mark,
    .collect = collect,
    .compact = compact
  };
  self->free = freeHook;
  return self;
}

int gc_nop_generation_count(int flags) {
  return 1;
}

bool gc_nop_use_fast_on_gen(int flags, int genID) {
  return true;
}
