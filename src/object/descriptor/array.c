#include "array.h"
#include "object/descriptor.h"
#include "util/util.h"
#include <string.h>

static int impl_forEachOffset(struct descriptor* super, struct object* object, int (^iterator)(size_t offset)) {
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

static size_t impl_getAlignment(struct descriptor* super) {
  return alignof(struct object*);
}

// Array don't have names
static const char* impl_getName(struct descriptor* super) {
  return NULL;
}

static void impl_runFinalizer(struct descriptor* super, struct object* obj) {
}

static void impl_free(struct descriptor* super) {
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
  struct array_descriptor* self = container_of(super, struct array_descriptor, super);
  return self->arrayInfo.elementDescriptor;
}

static struct descriptor_ops ops = {
  .forEachOffset = impl_forEachOffset,
  .free = impl_free,
  .getAlignment = impl_getAlignment,
  .getObjectSize = impl_getObjectSize,
  .getName = impl_getName,
  .getDescriptorAt = impl_getDescriptorAt,
  .runFinalizer = impl_runFinalizer,
  .isCompatible = impl_isCompatible
};

int array_descriptor_new(struct array_descriptor* self, struct descriptor* desc, size_t length) {
  self->arrayInfo.length = length;
  self->arrayInfo.elementDescriptor = desc;
  return descriptor_init(&self->super, OBJECT_ARRAY, &ops);
}
