#include <stdbool.h>

#include "api.h"
#include "heap.h"
#include "descriptor.h"

FLUFFYGC_EXPORT fluffygc_state* fluffygc_new_v1(
    size_t youngSize, size_t oldSize,
    size_t metaspaceSize,
    int localFrameStackSize,
    float concurrentOldGCthreshold) {
  return (fluffygc_state*) 
    heap_new(youngSize, 
      oldSize, 
      metaspaceSize, 
      localFrameStackSize, 
      concurrentOldGCthreshold);
}

FLUFFYGC_EXPORT void fluffygc_free(fluffygc_state* self) {
  heap_free((struct heap*) self);
}

/*
FLUFFYGC_EXPORT fluffygc_descriptor* fluffygc_descriptor_new_v1(
    fluffygc_state* self,
    fluffygc_typeid typeID,
    size_t objectSize,
    int numFields,
    fluffygc_field* fields) {
  struct descriptor_typeid typeID2 = {
    .ownerID = typeID.ownerID,
    .typeID = typeID.typeID,
    .name = typeID.name
  };
  return (fluffygc_descriptor*) heap_descriptor_new((struct heap*) self, typeID2, objectSize, numFields, (struct descriptor_field*) fields);
}

FLUFFYGC_EXPORT void fluffygc_descriptor_release(
    fluffygc_state* self, 
    fluffygc_descriptor* desc) {
  heap_descriptor_release((struct heap*) self, (struct descriptor*) desc);
}

FLUFFYGC_EXPORT bool fluffygc_attach_thread(
    fluffygc_state* self) {
  if (heap_is_attached((struct heap*) self))
    return false;
  
  return heap_attach_thread((struct heap*) self);
}

FLUFFYGC_EXPORT bool fluffygc_deattach_thread(
    fluffygc_state* self) {
  if (!heap_is_attached((struct heap*) self))
    return false;

  heap_detach_thread((struct heap*) self);
  return true;
}
*/



