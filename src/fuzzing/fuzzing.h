#ifndef _headers_1674625865_FluffyGC_fuzzing
#define _headers_1674625865_FluffyGC_fuzzing

#include <stddef.h>
#include <stdint.h>

#include "config.h"

int fuzzing_soc(const void* data, size_t size);
int fuzzing_heap(const void* data, size_t size);

#if IS_ENABLED(CONFIG_FUZZ_SOC)
# define fuzzing_fuzz fuzzing_soc
#endif

#if IS_ENABLED(CONFIG_FUZZ_HEAP)
# define fuzzing_fuzz fuzzing_heap
#endif

#endif

