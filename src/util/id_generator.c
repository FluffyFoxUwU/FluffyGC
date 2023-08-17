#include <stdatomic.h>
#include <stdint.h>

#include "panic.h"
#include "id_generator.h"

uint64_t id_generator_get(struct id_generator_state* self) {
  uint64_t id = atomic_fetch_add(&self->current, 1);
  if (id == UINT64_MAX)
    panic("ID is exhausted!!!");
  return id;
}


