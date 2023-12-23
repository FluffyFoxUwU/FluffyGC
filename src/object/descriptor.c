#include <stdatomic.h>
#include <stdio.h>

#include "FluffyHeap.h"
#include "bug.h"
#include "descriptor.h"
#include "object/object.h"
#include "descriptor.h"
#include "util/counter.h"
#include "managed_heap.h"

int descriptor_init(struct descriptor* self, enum object_type type, struct descriptor_ops* ops, fh_type_info* typeInfo) {
  *self = (struct descriptor) {
    .ops = ops,
    .api.typeInfo = *typeInfo
  };
  counter_init(&self->directUsageCounter);
  self->type = type;
  self->foreverUniqueID = managed_heap_generate_descriptor_id();
  return 0;
}

void descriptor_free(struct descriptor* self) {
  if (!self)
    return;
  
  self->ops->free(self);
}

int descriptor_for_each_offset(struct object* object, int (^iterator)(size_t offset)) {
  return object->movePreserve.descriptor->ops->forEachOffset(object->movePreserve.descriptor, object, iterator);
}

int descriptor_is_assignable_to(struct object* self, size_t offset, struct descriptor* b) {
  struct descriptor* a = self->movePreserve.descriptor->ops->getDescriptorAt(self->movePreserve.descriptor, offset);
  BUG_ON(b->type == OBJECT_UNMAKEABLE);
  
  // OBJECT_UNMAKEABLE is special in that it
  // may determine special behaviour like
  // DESCRIPTOR_UNMAKEABLE_ANY_MARKER where
  // it unconditionally return true
  if (a->type == OBJECT_UNMAKEABLE)
    return a->ops->isCompatible(a, b);
  
  if (a->type != b->type)
    return false;
  return a->ops->isCompatible(a, b);
}

void descriptor_acquire(struct descriptor* self) {
  counter_increment(&self->directUsageCounter);
}

void descriptor_release(struct descriptor* self) {
  if (counter_decrement(&self->directUsageCounter) > 1)
    return;
}

void descriptor_init_object(struct descriptor* self, struct object* obj) {
  self->ops->forEachOffset(self, obj, ^int (size_t offset) {
    atomic_init((_Atomic(struct object*)*) ((char*) obj->dataPtr.ptr + offset), NULL);
    return 0;
  });
  
  self->ops->postInitObject(self, obj);
}

size_t descriptor_get_object_size(struct descriptor* self) {
  return self->ops->getObjectSize(self);
}

size_t descriptor_get_alignment(struct descriptor* self) {
  return self->ops->getAlignment(self);
}

const char* descriptor_get_name(struct descriptor* self) {
  return self->ops->getName(self);
}

void descriptor_run_finalizer_on(struct descriptor* self, struct object* obj) {
  return self->ops->runFinalizer(self, obj);
}

ssize_t descriptor_calc_offset(struct descriptor* self, size_t index) {
  return self->ops->calcOffset(self, index);
}

const char* descriptor_object_type_tostring(enum object_type type) {
  switch (type) {
    case OBJECT_NORMAL:
      return "normal";
    case OBJECT_ARRAY:
      return "array";
    case OBJECT_UNMAKEABLE:
      return "unmakeable";
  }
  return NULL;
}
