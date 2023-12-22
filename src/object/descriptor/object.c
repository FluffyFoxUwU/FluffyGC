#include <pthread.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "macros.h"
#include "FluffyHeap.h"
#include "api/api.h"
#include "api/hooks.h"
#include "context.h"
#include "object/descriptor.h"
#include "object.h"
#include "object/object.h"
#include "util/util.h"
#include "vec.h"

static int compareByOffset(const void* _a, const void* _b) {
  const struct object_descriptor_field* a = _a;
  const struct object_descriptor_field* b = _b;

  if (a->offset < b->offset)
    return -1;
  else if (a->offset > b->offset)
    return 1;
  else
    return 0;
}

static void freeSelf(struct descriptor* super) {
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  object_descriptor_free(self);
}

void object_descriptor_free(struct object_descriptor* self) {
  api_on_object_descriptor_free(self);
  vec_deinit(&self->fields);
  free(self);
}

static int getIndexFromOffset(struct object_descriptor* self, size_t offset) {
  struct object_descriptor_field toSearch = {
    .offset = offset,
  };

  struct object_descriptor_field* result = bsearch(&toSearch, self->fields.data, self->fields.length, sizeof(*self->fields.data), compareByOffset);
  if (!result)
    return -ESRCH;
  return indexof(self->fields.data, result);
}

static int impl_forEachOffset(struct descriptor* super, struct object* object, int (^iterator)(size_t offset)) {
  UNUSED(object);
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  int i;
  struct object_descriptor_field* field;
  
  int ret = 0;
  vec_foreach_ptr(&self->fields, field, i)
    if ((ret = iterator(field->offset)) != 0)
      break;
  return ret;
}

static size_t impl_getObjectSize(struct descriptor* super) {
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  return self->objectSize;
}

static size_t impl_getAlignment(struct descriptor* super) {
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  return self->alignment;
}

static const char* impl_getName(struct descriptor* super) {
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  return self->name;
}

static void impl_runFinalizer(struct descriptor* super, struct object* obj) {
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  self->api.param.finalizer(obj);
}

static struct descriptor* impl_getDescriptorAt(struct descriptor* super, size_t offset) {
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  int index;
  index = getIndexFromOffset(self, offset);
  BUG_ON(index < 0);
  return self->fields.data[index].dataType;
}

// Ehh, just simple a == b UwU
static bool impl_isCompatible(struct descriptor* a, struct descriptor* b) {
  return a == b;
}

static void impl_postInitObject(struct descriptor* super, struct object* obj) {
  obj->movePreserve.descriptor = super;
}

static ssize_t impl_calcOffset(struct descriptor* super, size_t index) {
  struct object_descriptor* self = container_of(super, struct object_descriptor, super);
  if (index >= (size_t) self->fields.length)
    return -1;
  return self->fields.data[index].offset;
}

static struct descriptor_ops ops = {
  .forEachOffset = impl_forEachOffset,
  .free = freeSelf,
  .getAlignment = impl_getAlignment,
  .getObjectSize = impl_getObjectSize,
  .getName = impl_getName,
  .getDescriptorAt = impl_getDescriptorAt,
  .isCompatible = impl_isCompatible,
  .runFinalizer = impl_runFinalizer,
  .postInitObject = impl_postInitObject,
  .calcOffset = impl_calcOffset
};

void object_descriptor_init(struct object_descriptor* self) {
  qsort(self->fields.data, self->fields.length, sizeof(*self->fields.data), compareByOffset); 
  *self->super.ops = ops;
}

struct object_descriptor* object_descriptor_new() {
  struct object_descriptor* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct object_descriptor) {}; 
  vec_init(&self->fields);
  
  
  fh_type_info typeInfo = {};
  typeInfo.type = FH_TYPE_NORMAL;
  typeInfo.info.normal = API_EXTERN(&self->super);
  
  if (descriptor_init(&self->super, OBJECT_NORMAL, &ops, &typeInfo) < 0)
    goto failure;
  return self;

failure:
  freeSelf(&self->super);
  return NULL;
}
