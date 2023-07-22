#include <stdatomic.h>
#include <stdint.h>

#include "panic.h"
#include "id_generator.h"

static _Atomic(uint64_t) current = 1;

uint64_t id_generator_get() {
  uint64_t id = atomic_fetch_add(&current, 1);
  if (id == UINT64_MAX)
    panic("ID is exhausted!!!");
  return id;
}


