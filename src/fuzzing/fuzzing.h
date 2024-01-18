#ifndef _headers_1674625865_FluffyGC_fuzzing
#define _headers_1674625865_FluffyGC_fuzzing

#include <stddef.h>

#include "attributes.h"
#include "config.h"

int fuzzing_soc(const void* data, size_t size);
int fuzzing_heap(const void* data, size_t size);
int fuzzing_root_refs(const void* data, size_t size);

ATTRIBUTE_USED()
static int fuzzing_blank(const void*, size_t) {
  return 0;
}

#if IS_ENABLED(CONFIG_FUZZ_SOC)
# define fuzzing_fuzz fuzzing_soc
#elif IS_ENABLED(CONFIG_FUZZ_HEAP)
# define fuzzing_fuzz fuzzing_heap
#elif IS_ENABLED(CONFIG_FUZZ_ROOT_REFS)
# define fuzzing_fuzz fuzzing_root_refs
#else
# define fuzzing_fuzz fuzzing_blank
#endif

#endif

