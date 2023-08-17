#ifndef _headers_1689987330_FluffyGC_id_generator
#define _headers_1689987330_FluffyGC_id_generator

#include <stdint.h>

struct id_generator_state {
  _Atomic(uint64_t) current;
};

// 0 and -1 never be valid ID
// wraps around if necessary but
// practically 64-bit has alot of IDs
// which can take hundreds of years to exhaust
uint64_t id_generator_get(struct id_generator_state* self);

#define ID_GENERATOR_INIT {.current = 1}

#endif

