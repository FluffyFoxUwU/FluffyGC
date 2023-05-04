#include <stdlib.h>

#include "gc/gc.h"
#include "gc/nop/nop.h"
#include "util/util.h"

static void freeHook(struct gc_hooks* self) {
  free(self);
}

struct gc_hooks* gc_nop_new(int flags) {
  struct gc_hooks* self = malloc(sizeof(*self));
  *self = (struct gc_hooks) {GC_HOOKS_DEFAULT};
  self->free = freeHook;
  return self;
}