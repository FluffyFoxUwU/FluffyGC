#ifndef header_1655960483_asan_stuff_h
#define header_1655960483_asan_stuff_h

#include <stddef.h>

// No-op if ASAN disabled or unavailable
void asan_poison_memory_region(volatile void* address, size_t sz);
void asan_unpoison_memory_region(volatile void* address, size_t sz);

#endif

