#ifndef _headers_1687668619_FluffyGC_array
#define _headers_1687668619_FluffyGC_array

#include "FluffyHeap.h"
#include "object/descriptor.h"

struct array_descriptor {
  struct descriptor super;
  
  // Don't forget to change corresponding impl_isCompatibles
  struct {
    struct descriptor* elementDescriptor;
    size_t length;
  } arrayInfo;
  
  struct {
    fh_ref_array_info refArrayInfo;
  } api;
};

// Array descriptors are rather embedded
// as value
int array_descriptor_init(struct array_descriptor* self, struct descriptor* elementType, size_t length);

#endif

