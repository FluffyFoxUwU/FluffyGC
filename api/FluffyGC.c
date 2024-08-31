#include <stddef.h>

#include "api/headers/FluffyGC.h"
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


