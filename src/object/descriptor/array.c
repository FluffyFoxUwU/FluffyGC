#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "macros.h"
#include "FluffyHeap.h"
#include "api/api.h"
#include "array.h"
#include "object/descriptor.h"
#include "object/object.h"
#include "panic.h"
#include "util/util.h"

static_assert(sizeof(struct object*) == alignof(struct object*), "This destroys the offset calculation if this fails");

static int impl_forEachOffset(struct descriptor* super, struct object* object, int (^iterator)(size_t offset)) {
  UNUSED(object);
  int ret = 0;
  struct array_descriptor* self = container_of(super, struct array_descriptor, super);
  for (size_t i = 0; i < self->arrayInfo.length; i++)
    if ((ret = iterator(sizeof(struct object*) * i)) != 0)
      break;
  return ret;
}

static size_t impl_getObjectSize(struct descriptor* super) {
  struct array_descriptor* self = container_of(super, struct array_descriptor, super);
  return self->arrayInfo.length * sizeof(struct object*);
}

static const char* impl_getName(struct descriptor* super) {
  struct array_descriptor* self = container_of(super, struct array_descriptor, super);
  static thread_local char buffer[64 * 1024];
  char* localBuffer = malloc(sizeof(buffer));
  if (!localBuffer) {
    snprintf(buffer, sizeof(buffer), "<Array type, insufficient memory for type name>[%zu]", self->arrayInfo.length);
    return buffer;
  }
  
  // This may recurse in case of multi dimensional array
  // and Fox couldn't think of better way than allocating
  // local buffer and strncpy back to primary buffer
  snprintf(localBuffer, sizeof(buffer), "%s[%zu]", descriptor_get_name(self->arrayInfo.elementDescriptor), self->arrayInfo.length);
  
  strncpy(buffer, localBuffer, sizeof(buffer));
  free(localBuffer);
  return buffer;
}

static void impl_runFinalizer(struct descriptor* super, struct object* obj) {
  UNUSED(super);
  UNUSED(obj);
}

static void impl_free(struct descriptor* super) {
  UNUSED(super);
  panic("Array descriptors are by value, free call is invalid");
}

static bool impl_isCompatible(struct descriptor* _a, struct descriptor* _b) {
  struct array_descriptor* a = container_of(_a, struct array_descriptor, super);
  struct array_descriptor* b = container_of(_b, struct array_descriptor, super);
  
  // I'm genuinely hate that there no easy comparison
  // return memcmp(&a->arrayInfo, &b->arrayInfo, sizeof(b->arrayInfo)) == 0;
  return (a->arrayInfo.elementDescriptor == b->arrayInfo.elementDescriptor) &&
         (a->arrayInfo.length == b->arrayInfo.length);
}

static struct descriptor* impl_getDescriptorAt(struct descriptor* super, size_t offset) {
  UNUSED(offset);
  struct array_descriptor* self = container_of(super, struct array_descriptor, super);
  return self->arrayInfo.elementDescriptor;
}

static void impl_postObjectInit(struct descriptor* super, struct object* obj) {
  struct array_descriptor* self = container_of(super, struct array_descriptor, super);
  
  // Array descriptor is by value therefore copy
  obj->movePreserve.embedded.array = *self;
  
  // Updates pointer to refer to new instance of itself
  obj->movePreserve.embedded.array.super.api.typeInfo.info.refArray = &obj->movePreserve.embedded.array.api.refArrayInfo;
  
  obj->movePreserve.descriptor = &obj->movePreserve.embedded.array.super;
}

static ssize_t impl_calcOffset(struct descriptor* super, size_t index) {
  struct array_descriptor* self = container_of(super, struct array_descriptor, super);
  if (index >= self->arrayInfo.length)
    return -1;
  return index * sizeof(struct object*);
}

static struct descriptor_ops ops = {
  .forEachOffset = impl_forEachOffset,
  .free = impl_free,
  .getObjectSize = impl_getObjectSize,
  .getName = impl_getName,
  .getDescriptorAt = impl_getDescriptorAt,
  .runFinalizer = impl_runFinalizer,
  .isCompatible = impl_isCompatible,
  .postInitObject = impl_postObjectInit,
  .calcOffset = impl_calcOffset
};

int array_descriptor_init(struct array_descriptor* self, struct descriptor* desc, size_t length) {
  self->arrayInfo.length = length;
  self->arrayInfo.elementDescriptor = desc;
  
  self->api.refArrayInfo.length = length;
  self->api.refArrayInfo.elementDescriptor = API_EXTERN(desc);
  
  fh_type_info typeInfo = {};
  typeInfo.type = FH_TYPE_ARRAY;
  typeInfo.info.refArray = &self->api.refArrayInfo;
  return descriptor_init(&self->super, OBJECT_ARRAY, &ops, &typeInfo);
}


