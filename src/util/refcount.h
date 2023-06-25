#ifndef _headers_1673683913_FluffyGC_resourc_tracker
#define _headers_1673683913_FluffyGC_resourc_tracker

#include <stdatomic.h>
#include <stdbool.h>

#include "attributes.h"
#include "util/counter.h"

struct refcount {
  struct counter counter;
};

ATTRIBUTE_USED()
static inline void refcount_init(struct refcount* self) {
  counter_init(&self->counter);
  counter_increment(&self->counter);
}

ATTRIBUTE_USED()
static inline void refcount_acquire(struct refcount* self) {
  counter_increment(&self->counter);
}

// false if losing last reference
ATTRIBUTE_USED()
static inline bool refcount_release(struct refcount* self) {
  return counter_decrement(&self->counter) > 1;
}

#endif

