#ifndef _headers_1687590666_FluffyGC_counter
#define _headers_1687590666_FluffyGC_counter

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "attributes.h"

struct counter {
  _Atomic(uint64_t) counter;
};

ATTRIBUTE_USED()
static inline void counter_init(struct counter* self) {
  atomic_init(&self->counter, 0);
}

ATTRIBUTE_USED()
static inline uint64_t counter_increment(struct counter* self) {
  return atomic_fetch_add(&self->counter, 1);
}

ATTRIBUTE_USED()
static inline uint64_t counter_decrement(struct counter* self) {
  return atomic_fetch_sub(&self->counter, 1);
}

#endif

