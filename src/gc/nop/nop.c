#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "gc/gc.h"
#include "gc/nop/nop.h"
#include "util/util.h"
#include "managed_heap.h"
#include "macros.h"

int gc_nop_mark(struct generation* gen) { 
  UNUSED(gen);
  return 0;
}

size_t gc_nop_collect(struct generation* gen) {
  UNUSED(gen);
  return 0;
}

void gc_nop_compact(struct generation* gen) {
  UNUSED(gen);
}

void gc_nop_start_cycle(struct generation* gen) {
  UNUSED(gen);
}

void gc_nop_end_cycle(struct generation* gen) {
  UNUSED(gen);
}

void gc_nop_free(struct gc_ops* ops) {
  UNUSED(ops);
}

static struct gc_ops hooks = { GC_HOOKS_DEFAULT,
  .collect = gc_nop_collect,
  .free = gc_nop_free
};

struct gc_ops* gc_nop_new(gc_flags flags) {
  UNUSED(flags);
  return &hooks;
}

int gc_nop_generation_count(gc_flags flags) {
  UNUSED(flags);
  return 1;
}

bool gc_nop_use_fast_on_gen(gc_flags flags, int genID) {
  UNUSED(flags);
  UNUSED(genID);
  return true;
}

// Wont to anywhere *squeak*
size_t gc_nop_preferred_promotion_size(gc_flags flags, int genID) {
  UNUSED(flags);
  UNUSED(genID);
  return SIZE_MAX;
}

int gc_nop_preferred_promotion_age(gc_flags flags, int genID) {
  UNUSED(flags);
  UNUSED(genID);
  return INT_MAX;
}


