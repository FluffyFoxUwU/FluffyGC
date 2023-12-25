#ifndef _headers_1689936095_FluffyGC_common
#define _headers_1689936095_FluffyGC_common

// Common checks here

#include "FluffyHeap/FluffyHeap.h"

enum debug_access {
  DEBUG_ACCESS_READ,
  DEBUG_ACCESS_WRITE,
  DEBUG_ACCESS_RW
};

bool debug_check_access(fh_object* obj, size_t offset, size_t size, enum debug_access access);
const char* debug_get_unique_name(fh_object* obj);

#endif

