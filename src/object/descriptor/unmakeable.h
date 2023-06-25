#ifndef _headers_1687241246_FluffyGC_unmakeable
#define _headers_1687241246_FluffyGC_unmakeable

#include "object/descriptor.h"

enum unmakeable_descriptor_type {
  DESCRIPTOR_UNMAKEABLE_ANY_MARKER
};

struct unmakeable_descriptor {
  struct descriptor super;
  
  enum unmakeable_descriptor_type type;
  const char* name;
};

extern struct descriptor_ops unmakeable_ops;
#define UNMAKEABLE_DESCRIPTOR_DEFINE(varName, _name, _type) \
  static struct unmakeable_descriptor varName = { \
    .super = { \
      .type = OBJECT_UNMAKEABLE, \
      .ops = &unmakeable_ops \
    }, \
    .name = _name, \
    .type = _type \
  }; 

#endif

