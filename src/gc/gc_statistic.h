#ifndef _headers_1685716400_FluffyGC_gc_statistic
#define _headers_1685716400_FluffyGC_gc_statistic

#include <stddef.h>
#include <stdint.h>

struct gc_statistic {
  size_t reclaimedBytes;
  size_t promotedBytes;
  uint64_t promotedObjects;
};

#endif

