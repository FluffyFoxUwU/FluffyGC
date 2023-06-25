#ifndef _headers_1687241246_FluffyGC_unmakeable
#define _headers_1687241246_FluffyGC_unmakeable

#include "object/descriptor.h"

struct array_descriptor {
  struct descriptor super;
  struct descriptor* elementInfo;
};

// Array internally caches same descriptor to reuse
// to avoid creating new descriptor for each array

struct array_descriptor* array_descriptor_new();

#endif

