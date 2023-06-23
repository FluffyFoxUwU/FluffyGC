#include "descriptor.h"
#include "bug.h"
#include "object/descriptor/unmakeable.h"
#include "object/object.h"
#include "panic.h"
#include "descriptor.h"
#include "util/refcount.h"
#include "util/util.h"

int descriptor_init(struct descriptor* self, enum object_type type, struct descriptor_ops* ops) {
  *self = (struct descriptor) {
    .ops = ops
  };
  refcount_init(&self->usages);
  self->type = type;
  return 0;
}

void descriptor_free(struct descriptor* self) {
  if (!self)
    return;
  
  self->ops->free(self);
}

void descriptor_for_each_offset(struct object* object, void (^iterator)(size_t offset)) {
  return object->movePreserve.descriptor->ops->forEachOffset(object->movePreserve.descriptor, object, iterator);
}

int descriptor_is_assignable_to(struct object* self, size_t offset, struct descriptor* b) {
  struct descriptor* a = self->movePreserve.descriptor->ops->getDescriptorAt(self->movePreserve.descriptor, offset);
  if (a->type != OBJECT_UNMAKEABLE_STATIC)
    return a == b;
  
  struct unmakeable_descriptor* unmakeable = container_of(a, struct unmakeable_descriptor, super);
  switch (unmakeable->type) {
    case DESCRIPTOR_UNMAKEABLE_ANY_MARKER:
      return true;
  }
  
  panic();
}

void descriptor_acquire(struct descriptor* self) {
  refcount_acquire(&self->usages);
}

void descriptor_release(struct descriptor* self) {
  if (refcount_release(&self->usages))
    return;
  panic();
}

void descriptor_init_object(struct descriptor* self, struct object* obj) {
  self->ops->initObject(self, obj);
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
