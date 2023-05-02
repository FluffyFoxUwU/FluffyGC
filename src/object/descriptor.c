#include <pthread.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "context.h"
#include "descriptor.h"
#include "object.h"
#include "util/refcount.h"
#include "util/util.h"

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

struct descriptor* descriptor_new(struct descriptor_typeid id, size_t objectSize, int numFields, struct descriptor_field* offsets) {
  struct descriptor* self = malloc(sizeof(*self) + sizeof(*offsets) * numFields);
  if (!self)
    return NULL;
  
  self->numFields = numFields;
  refcount_init(&self->refcount);
  self->id = id;
  self->id.name = strdup(id.name);
  self->objectSize = objectSize;

  for (int i = 0; i < numFields; i++) {
    self->fields[i] = offsets[i];
    self->fields[i].name = strdup(offsets[i].name);
  }

  qsort(self->fields, numFields, sizeof(*self->fields), compareByOffset); 
  return self;
}

void descriptor_acquire(struct descriptor* self) {
  refcount_acquire(&self->refcount);
}

void descriptor_release(struct descriptor* self) {
  if (refcount_release(&self->refcount))
    return;

  for (int i = 0; i < self->numFields; i++)
    free((void*) self->fields[i].name);
  free((void*) self->id.name);
  free(self);
}

void descriptor_init(struct descriptor* self, struct object* obj) {
  for (int i = 0; i < self->numFields; i++)
    descriptor_write_ptr(self, obj, i, NULL);
}

void descriptor_write_ptr(struct descriptor* self, struct object* data, int index, struct object* ptr) {
  object_write_reference(data, self->fields[index].offset, ptr);
}

struct root_ref* descriptor_read_ptr(struct descriptor* self, struct object* data, int index) {
  return object_read_reference(data, self->fields[index].offset);
}

int descriptor_get_index_from_offset(struct descriptor* self, size_t offset) {
  struct descriptor_field toSearch = {
    .offset = offset,
  };

  struct descriptor_field* result = bsearch(&toSearch, self->fields, self->numFields, sizeof(*self->fields), compareByOffset);
  if (!result)
    return -ESRCH;
  return indexof(self->fields, result);
}



