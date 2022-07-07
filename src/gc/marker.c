#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

#include "marker.h"
#include "../reference.h"
#include "../heap.h"
#include "../region.h"
#include "../descriptor.h"

struct mark_context {
  struct region* onlyIn;
  struct object_info* objectInfo;
};

static struct mark_context makeContext(struct region* onlyIn, struct object_info* objectInfo) {
  struct mark_context tmp = {
    .onlyIn = onlyIn,
    .objectInfo = objectInfo
  };
  return tmp;
}

static void mark(const struct mark_context self);
static void markNormalObject(const struct mark_context self) {
  struct descriptor* desc = self.objectInfo->typeSpecific.normal.desc;

  for (int i = 0; i < desc->numFields; i++) {
    size_t offset = desc->fields[i].offset;
    void* ptr;
    region_read(self.objectInfo->regionRef->owner, self.objectInfo->regionRef, offset, &ptr, sizeof(void*));
    if (!ptr)
      continue;
    
    struct region_reference* ref = heap_get_region_ref(self.objectInfo->owner, ptr);
    assert(ref);
    struct object_info* objectInfo = heap_get_object_info(self.objectInfo->owner, ref);
    assert(objectInfo);
    
    if (ref->owner != self.onlyIn)
      continue;

    //printf("[Marking: Ref: %p] Name: '%s' (offset: %zu  ptr: %p)\n", self.objectInfo->regionRef, desc->fields[i].name, offset, ptr);
    
    mark(makeContext(self.onlyIn, objectInfo));
  }
}

static void markArrayObject(const struct mark_context self) {
  void** array = self.objectInfo->regionRef->data;
  for (int i = 0; i < self.objectInfo->typeSpecific.pointerArray.size; i++) {
    struct region_reference* ref = heap_get_region_ref(self.objectInfo->owner, array[i]);
    assert(ref);
    struct object_info* objectInfo = heap_get_object_info(self.objectInfo->owner, ref);
    assert(objectInfo);
    
    if (ref->owner != self.onlyIn)
      continue;

    //printf("[Marking: Ref: %p] Name: '%s' (offset: %zu  ptr: %p)\n", self.objectInfo->regionRef, desc->fields[i].name, offset, ptr);
    
    mark(makeContext(self.onlyIn, objectInfo));
  } 
}

static void mark(const struct mark_context self) {
  if (self.objectInfo->regionRef->owner == self.onlyIn || self.onlyIn == NULL)
    if (atomic_exchange(&self.objectInfo->isMarked, true) == true)
      return;

  switch (self.objectInfo->type) {
    case OBJECT_TYPE_NORMAL:
      markNormalObject(self);
      break;
    case OBJECT_TYPE_ARRAY:
      markArrayObject(self);
      break;
    case OBJECT_TYPE_OPAQUE:
      break;
    case OBJECT_TYPE_UNKNOWN:
      abort();
  }
}

void gc_marker_mark(struct region* onlyIn, struct object_info* objectInfo) {
  mark(makeContext(onlyIn, objectInfo));
}


