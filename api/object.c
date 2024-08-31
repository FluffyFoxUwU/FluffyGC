#include <stddef.h>

#include <FluffyGC/FluffyGC.h>
#include <FluffyGC/export.h>
#include <FluffyGC/object.h>
#include <FluffyGC/descriptor.h>

#include "api/common.h"
#include "heap/heap.h"

FLUFFYGC_EXPORT
void* fluffygc_object_get_data_ptr(fluffygc_object* obj) {
  return API_INTERN(obj)->obj->data;
}

FLUFFYGC_EXPORT
fluffygc_object* fluffygc_object_new(fluffygc_state* state, const fluffygc_descriptor* desc, size_t extraSize) {
  return API_EXTERN(heap_alloc_with_descriptor(API_INTERN(state), API_INTERN(desc), extraSize));
}

FLUFFYGC_EXPORT
void fluffygc_object_unref(fluffygc_state* state, fluffygc_object* obj) {
  heap_root_unref(API_INTERN(state), API_INTERN(obj));
}
