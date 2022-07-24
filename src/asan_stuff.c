#include <assert.h>
#include <stdint.h>

#include "asan_stuff.h"
#include "config.h"

#if IS_ENABLED(CONFIG_ASAN)
# include <sanitizer/asan_interface.h>
#endif

void asan_poison_memory_region(volatile void* address, size_t sz) {
  assert(((uintptr_t) address) % 8 == 0);
  assert(((uintptr_t) (address + sz)) % 8 == 0);
# if IS_ENABLED(CONFIG_ASAN)
  ASAN_POISON_MEMORY_REGION(address, sz);
# endif
}
void asan_unpoison_memory_region(volatile void* address, size_t sz) {
  assert(((uintptr_t) address) % 8 == 0);
  assert(((uintptr_t) (address + sz)) % 8 == 0);
# if IS_ENABLED(CONFIG_ASAN)
  ASAN_UNPOISON_MEMORY_REGION(address, sz);
# endif
}

