#include "unmakeable.h"
#include "object/descriptor.h"
#include "panic.h"

static void impl_initObject(struct descriptor* super, struct object* obj) {
  panic("Operation inappropriate for unmakeable!");
}

static void impl_forEachOffset(struct descriptor* super, struct object* object, void (^iterator)(size_t offset)) {
  panic("Operation inappropriate for unmakeable!");
}

static size_t impl_getObjectSize(struct descriptor* super) {
  panic("Operation inappropriate for unmakeable!");
}

static size_t impl_getAlignment(struct descriptor* super) {
  panic("Operation inappropriate for unmakeable!");
}

static const char* impl_getName(struct descriptor* super) {
  struct unmakeable_descriptor* self = container_of(super, struct unmakeable_descriptor, super);
  return self->name;
}

static void impl_runFinalizer(struct descriptor* super, struct object* obj) {
  panic("Operation inappropriate for unmakeable!");
}

static void impl_free(struct descriptor* super) {
}

static struct descriptor* impl_getDescriptorAt(struct descriptor* super, size_t offset) {
  panic("Operation inappropriate for unmakeable!");
}

struct descriptor_ops unmakeable_ops = {
  .forEachOffset = impl_forEachOffset,
  .free = impl_free,
  .getAlignment = impl_getAlignment,
  .getObjectSize = impl_getObjectSize,
  .getName = impl_getName,
  .getDescriptorAt = impl_getDescriptorAt,
  .initObject = impl_initObject,
  .runFinalizer = impl_runFinalizer
};
