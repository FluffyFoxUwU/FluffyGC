#ifndef _headers_1686662453_FluffyGC_pre_code
#define _headers_1686662453_FluffyGC_pre_code

#include "attributes.h"

#define __FLUFFYHEAP_EXPORT ATTRIBUTE_USED() ATTRIBUTE((visibility("default"))) extern

#include "api.h"

#define INTERN(c) API_INTERN(c)

#define EXTERN(c) API_EXTERN(c)

// Arrays need explicit cast (and also checks whether
// its appropriately fh_object* to ensure safe cast
// but caller must ensure its compatible from generic 
// fh_object)
#define CAST_TO_ARRAY(c) _Generic ((c), \
  fh_object*: (fh_array*) (c) \
)

#endif

