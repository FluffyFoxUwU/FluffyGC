#ifndef _headers_1673683913_FluffyGC_resourc_tracker
#define _headers_1673683913_FluffyGC_resourc_tracker

#include <stdatomic.h>
#include <stdbool.h>

#include "compiler_config.h"

struct refcount {
  atomic_uint usage;
};

ATTRIBUTE_USED()
static inline void refcount_init(struct refcount* self) {
  atomic_init(&self->usage, 0);
}

ATTRIBUTE_USED()
static inline void refcount_acquire(struct refcount* self) {
  atomic_fetch_add(&self->usage, 1);
}

// false if losing last reference
ATTRIBUTE_USED()
static inline bool refcount_release(struct refcount* self) {
  return atomic_fetch_sub(&self->usage, 1) > 1;
}

#endif

