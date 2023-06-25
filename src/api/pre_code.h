#ifndef _headers_1686662453_FluffyGC_pre_code
#define _headers_1686662453_FluffyGC_pre_code

#include "attributes.h"

#define __FLUFFYHEAP_EXPORT ATTRIBUTE((used)) ATTRIBUTE((visibility("default"))) extern

#define INTERN(c) _Generic ((c), \
  fluffyheap*: (struct managed_heap*) (c), \
  fh_object*: (struct root_ref*) (c), \
  fh_context*: (struct context*) (c), \
  fh_descriptor*: (struct descriptor*) (c), \
  fh_array*: (struct root_ref*) (c) \
)

#define EXTERN(c) _Generic ((c), \
  struct managed_heap*: (fluffyheap*) (c), \
  struct root_ref*: (fh_object*) (c), \
  struct context*: (fh_context*) (c), \
  struct descriptor*: (fh_descriptor*) (c) \
)

// Arrays need explicit cast (and also checks whether
// its appropriately fh_object* to ensure safe cast)
#define CAST_TO_ARRAY(c) _Generic ((c), \
  fh_object*: (fh_array*) (c) \
)

#endif

