#include <stdlib.h>

#include "concurrency/rwlock.h"
#include "context.h"
#include "gc.h"
#include "gc/nop/nop.h"
#include "bug.h"
#include "config.h"

thread_local struct gc_struct* gc_current = NULL;

#define SELECT_GC(algo, ret, func, ...) \
  switch (algo) { \
    case GC_UNKNOWN: \
      BUG(); \
    case GC_NOP_GC: \
      ret = gc_nop_ ## func(__VA_ARGS__); \
      break; \
  }

#define SELECT_GC_VOID(algo, func, ...) \
  switch (algo) { \
    case GC_UNKNOWN: \
      BUG(); \
    case GC_NOP_GC: \
      gc_nop_ ## func(__VA_ARGS__); \
      break; \
  }

struct gc_struct* gc_new(enum gc_algorithm algo, int gcFlags) {
  struct gc_struct* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  SELECT_GC(algo, self->hooks, new, gcFlags);
  if (!self->hooks || rwlock_init(&self->gcLock) < 0)
    goto failure;
  
  return self;
failure:
  gc_free(self);
  return NULL;
}

int gc_generation_count(enum gc_algorithm algo, int gcFlags) {
  int ret;
  SELECT_GC(algo, ret, generation_count, gcFlags);
  return ret;
}

void gc_start(struct gc_struct* self, struct generation* generation, size_t freeSizeHint) {
  self->hooks->mark(generation);
  self->hooks->collect(generation, freeSizeHint);
  if (IS_ENABLED(CONFIG_GC_COMPACTING))
    self->hooks->compact(generation, freeSizeHint);
}

void gc_free(struct gc_struct* self) {
  self->hooks->free(self->hooks);
  rwlock_cleanup(&self->gcLock);
  free(self);
}

bool gc_use_fast_on_gen(enum gc_algorithm algo, int gcFlags, int genID) {
  bool ret;
  SELECT_GC(algo, ret, use_fast_on_gen, gcFlags, genID);
  return ret;
}
