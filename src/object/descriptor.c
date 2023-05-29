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
  int i;
  struct descriptor_field* field;
  vec_foreach_ptr(&self->fields, field, i)
    free((void*) field->name);
  
  vec_deinit(&self->fields);
  free((void*) self->id.name);
  free(self);
}

struct descriptor* descriptor_new(struct descriptor_type* type) {
  struct descriptor* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct descriptor) {}; 
  
  self->id = type->id;
  self->objectSize = type->objectSize;
  self->alignment = type->alignment;
  
  vec_init(&self->fields);
  refcount_init(&self->refcount);
  
  self->id.name = strdup(type->id.name);
  if (!self->id.name)
    goto failure;

  for (int i = 0; type->fields[i].name != NULL; i++) {
    vec_push(&self->fields, type->fields[i]);
    self->fields.data[i].name = strdup(type->fields[i].name);
    
    if (!self->fields.data[i].name)
      goto failure;
  }

  qsort(self->fields.data, self->fields.length, sizeof(*self->fields.data), compareByOffset); 
  return self;

failure:
  descriptor_free(self);
  return NULL;
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
  for (int i = 0; i < self->fields.length; i++)
    descriptor_write_ptr(self, obj, i, NULL);
}

void descriptor_write_ptr(struct descriptor* self, struct object* data, int index, struct object* ptr) {
  object_write_reference(data, self->fields.data[index].offset, ptr);
}

struct root_ref* descriptor_read_ptr(struct descriptor* self, struct object* data, int index) {
  return object_read_reference(data, self->fields.data[index].offset);
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



