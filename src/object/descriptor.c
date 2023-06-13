#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "api/hooks.h"
#include "bug.h"
#include "context.h"
#include "descriptor.h"
#include "object.h"
#include "util/refcount.h"
#include "util/util.h"
#include "vec.h"

static int compareByOffset(const void* _a, const void* _b) {
  const struct descriptor_field* a = _a;
  const struct descriptor_field* b = _b;

  if (a->offset < b->offset)
    return -1;
  else if (a->offset > b->offset)
    return 1;
  else
    return 0;
}

static void descriptor_free(struct descriptor* self) {
  api_on_descriptor_free(self);
  free(self);
}

struct descriptor* descriptor_new() {
  struct descriptor* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct descriptor) {}; 
  vec_init(&self->fields);
  refcount_init(&self->refcount);
  
  return self;
}

void descriptor_init(struct descriptor* self) {
  qsort(self->fields.data, self->fields.length, sizeof(*self->fields.data), compareByOffset); 
}

void descriptor_acquire(struct descriptor* self) {
  refcount_acquire(&self->refcount);
}

void descriptor_release(struct descriptor* self) {
  if (refcount_release(&self->refcount))
    return;
  descriptor_free(self);
}

void descriptor_init_object(struct descriptor* self, struct object* obj) {
  switch (self->objectType) {
    case OBJECT_NORMAL: {
      int i;
      struct descriptor_field* field;
      vec_foreach_ptr(&self->fields, field, i)
        atomic_init((_Atomic(struct object*)*) (obj->dataPtr.ptr + field->offset), NULL);
      break;
    }
  }
}

int descriptor_get_index_from_offset(struct descriptor* self, size_t offset) {
  struct descriptor_field toSearch = {
    .offset = offset,
  };

  struct descriptor_field* result = bsearch(&toSearch, self->fields.data, self->fields.length, sizeof(*self->fields.data), compareByOffset);
  if (!result)
    return -ESRCH;
  return indexof(self->fields.data, result);
}

void descriptor_for_each_field(struct object* self, void (^iterator)(struct object* obj,size_t offset)) {
  switch (self->descriptor->objectType) {
    case OBJECT_NORMAL: {
      int i;
      struct descriptor_field* field;
      vec_foreach_ptr(&self->descriptor->fields, field, i)
        iterator(atomic_load((_Atomic(struct object*)*) (self->dataPtr.ptr + field->offset)), field->offset);
      break;
    }
  }
}

int descriptor_is_assignable_to(struct descriptor* self, size_t offset, struct descriptor* b) {
  struct descriptor* a = NULL;
  int index;
  switch (self->objectType) {
    case OBJECT_NORMAL: {
      index = descriptor_get_index_from_offset(self, offset);
      BUG_ON(index < 0);
      break;
    }
  }
  a = self->fields.data[index].dataType;
  return a == b;
}
