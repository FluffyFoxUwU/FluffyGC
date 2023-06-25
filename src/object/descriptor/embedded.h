#ifndef _headers_1687671375_FluffyGC_embedded
#define _headers_1687671375_FluffyGC_embedded

#include "object/descriptor/array.h"

struct embedded_descriptor {
  union {
    struct array_descriptor array;
  };
};

#endif

