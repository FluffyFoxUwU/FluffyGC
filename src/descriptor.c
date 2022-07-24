#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "descriptor.h"
#include "region.h"
#include "binary_search.h"

static int compareByOffset(const void* _a, const void* _b) {
  const struct descriptor_field* a = _a;
  const struct descriptor_field* b = _b;

  if (a->offset > b->offset)
    return 1;
  else if (a->offset < b->offset)
    return -1;
  else
    return 0;
}

struct descriptor* descriptor_new(struct descriptor_typeid id, size_t objectSize, int numFields, struct descriptor_field* offsets) {
  struct descriptor* self = malloc(sizeof(*self) + sizeof(*offsets) * numFields);
  if (!self)
    return NULL;
  
  self->numFields = numFields;
  atomic_init(&self->alreadyUnregistered, false);
  atomic_init(&self->counter, 1);
  self->id = id;
  self->id.name = strdup(id.name);

  for (int i = 0; i < numFields; i++) {
    self->fields[i] = offsets[i];
    self->fields[i].name = strdup(offsets[i].name);
  }

  qsort(self->fields, numFields, sizeof(*self->fields), compareByOffset);
  
  self->objectSize = objectSize;
  return self;
}

void descriptor_acquire(struct descriptor* self) {
  atomic_fetch_add(&self->counter, 1);
}

bool descriptor_release(struct descriptor* self) {
  if (atomic_fetch_sub(&self->counter, 1) != 1)
    return false;

  for (int i = 0; i < self->numFields; i++)
    free((void*) self->fields[i].name);
  free((void*) self->id.name);
  free(self);

  return true;
}

void descriptor_init(struct descriptor* self, struct region* region, struct region_reference* data) {
  for (int i = 0; i < self->numFields; i++)
    descriptor_write_ptr(self, region, data, i, NULL);
}

void descriptor_write_ptr(struct descriptor* self, struct region* region, struct region_reference* data, int index, void* ptr) {
  region_write(region, data, self->fields[index].offset, &ptr, sizeof(void*));
}

void* descriptor_read_ptr(struct descriptor* self, struct region* region, struct region_reference* data, int index) {
  void* ptr = NULL;
  region_read(region, data, self->fields[index].offset, &ptr, sizeof(void*));
  return ptr;
}

int descriptor_get_index_from_offset(struct descriptor* self, size_t offset) {
  struct descriptor_field toSearch = {
    .offset = offset,
    .name = NULL,
    .type = DESCRIPTOR_FIELD_TYPE_UNKNOWN
  };

  return binary_search_do(self->fields, self->numFields, sizeof(*self->fields), compareByOffset, &toSearch);
}



