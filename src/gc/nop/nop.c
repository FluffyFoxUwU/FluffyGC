#include <stdlib.h>

#include "gc/gc.h"
#include "gc/nop/nop.h"
#include "util/util.h"
#include "managed_heap.h"

int gc_nop_mark(struct generation* gen) {
  return 0;
}

size_t gc_nop_collect(struct generation* gen) {
  return 0;
}

void gc_nop_compact(struct generation* gen) {
}

void gc_nop_start_cycle(struct generation*) {
}

void gc_nop_end_cycle(struct generation*) {
}

void gc_nop_free(struct gc_hooks*) {}

static struct gc_hooks hooks = { GC_HOOKS_DEFAULT,
  .collect = gc_nop_collect,
  .free = gc_nop_free
};

struct gc_hooks* gc_nop_new(gc_flags flags) {
  return &hooks;
}

int gc_nop_generation_count(gc_flags flags) {
  return 1;
}

bool gc_nop_use_fast_on_gen(gc_flags flags, int genID) {
  return true;
}
