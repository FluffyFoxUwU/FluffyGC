#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "api/hooks.h"
#include "context.h"
#include "object/descriptor.h"
#include "object_descriptor.h"
#include "object.h"
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

void object_descriptor_free(struct object_descriptor* self) {
  api_on_object_descriptor_free(self);
  vec_deinit(&self->fields);
  free(self);
}

struct object_descriptor* object_descriptor_new() {
  struct object_descriptor* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct object_descriptor) {}; 
  vec_init(&self->fields);
  
  if (!(self->parent = descriptor_new_for_object_type(self)))
    goto failure_no_dealloc_parent;
  
  return self;

failure_no_dealloc_parent:
  object_descriptor_free(self);
  return NULL;
}

void object_descriptor_init(struct object_descriptor* self) {
  qsort(self->fields.data, self->fields.length, sizeof(*self->fields.data), compareByOffset); 
}

void object_descriptor_init_object(struct object_descriptor* self, struct object* obj) {
  int i;
  struct object_descriptor_field* field;
  vec_foreach_ptr(&self->fields, field, i)
    atomic_init((_Atomic(struct object*)*) (obj->dataPtr.ptr + field->offset), NULL);
}

int object_descriptor_get_index_from_offset(struct object_descriptor* self, size_t offset) {
  struct object_descriptor_field toSearch = {
    .offset = offset,
  };

  struct object_descriptor_field* result = bsearch(&toSearch, self->fields.data, self->fields.length, sizeof(*self->fields.data), compareByOffset);
  if (!result)
    return -ESRCH;
  return indexof(self->fields.data, result);
}

struct descriptor* object_descriptor_get_at(struct object_descriptor* self, size_t offset) {
  int index;
  index = object_descriptor_get_index_from_offset(self, offset);
  BUG_ON(index < 0);
  return self->fields.data[index].dataType;
}

void object_descriptor_for_each_offset(struct object_descriptor* self, struct object* object, void (^iterator)(size_t offset)) {
  int i;
  struct object_descriptor_field* field;
  vec_foreach_ptr(&self->fields, field, i)
    iterator(field->offset);
}

size_t object_descriptor_get_object_size(struct object_descriptor* self) {
  return self->objectSize;
}

size_t object_descriptor_get_alignment(struct object_descriptor* self) {
  return self->alignment;
}

const char* object_descriptor_get_name(struct object_descriptor* self) {
  return self->name;
}
