#include <stddef.h>

#include <FluffyGC/FluffyGC.h>
#include <FluffyGC/export.h>

#include "api/common.h"
#include "heap/heap.h"

FLUFFYGC_EXPORT
fluffygc_state* fluffygc_new(size_t heapSize) {
  return API_EXTERN(heap_new(heapSize));
}

FLUFFYGC_EXPORT
void fluffygc_free(fluffygc_state* self) {
  heap_free(API_INTERN(self));
}

FLUFFYGC_EXPORT
fluffygc_version fluffygc_get_api_version() {
  return fluffygc_make_version(1, 0, 0);
}

FLUFFYGC_EXPORT
fluffygc_version fluffygc_get_impl_version() {
  return fluffygc_make_version(1, 0, 0);
}

FLUFFYGC_EXPORT
const char* fluffygc_get_impl_version_description() {
  return fluffygc_get_impl_short_version_description();
}

FLUFFYGC_EXPORT
const char* fluffygc_get_impl_short_version_description() {
  return "Foxie's GC library v1.0.0";
}


