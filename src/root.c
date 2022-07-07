#include <stdlib.h>
#include <assert.h>

#include "reference.h"
#include "region.h"
#include "root.h"
#include "config.h"

struct root* root_new(int size) {
  struct root* self = malloc(sizeof(*self));
  if (!self)
    goto failure;
  
  self->size = size;
  
  self->entries = malloc(size * sizeof(*self->entries));
  if (!self->entries)
    goto failure;
  
  for (int i = 0; i < size; i++) {
    self->entries[i].isValid = false;
    self->entries[i].owner = self;
  }
  
  return self;
  
  failure:
  root_free(self);
  return NULL;
}

struct root_reference* root_add(struct root* self, struct region_reference* ref) {
  assert(ref);

  // Find free entry
  // TODO: Make this not O(n) at worse
  //       (maybe some free space bitmap)
  int freePos = 0;
  for (; freePos < self->size; freePos++)
    if (!self->entries[freePos].isValid)
      break;
  
  // Cant find free space
  if (freePos >= self->size)
    return NULL;

  struct root_reference* rootRef = &self->entries[freePos];  
  rootRef->isValid = true;
  rootRef->data = ref;
  return rootRef;
}

static void remove_entry(struct root* self, struct root_reference* ref){
  assert(self == ref->owner);

  ref->isValid = false;
  ref->data = NULL;
}

void root_remove(struct root* self, struct root_reference* ref) {
  remove_entry(self, ref);
}

bool root_resize(struct root* self, int newSize) {
  struct root_reference* newEntries = realloc(self->entries, sizeof(*self->entries) * newSize);
  if (!newEntries)
    return false;
  self->entries = newEntries;

  for (int i = self->size; i < newSize; i++) {
    self->entries[i].isValid = false;
    self->entries[i].owner = self;
    self->entries[i].data = NULL;
  }

  self->entries = newEntries;
  return true;
}

void root_clear(struct root* self) {
  for (int i = 0; i < self->size; i++)
    root_remove(self, &self->entries[i]);
}

void root_free(struct root* self) {
  if (!self)
    return;
  
  struct root_reference* current = &self->entries[0];
  for (int i = 0; i < self->size; i++) {
    remove_entry(self, current);
    current = &self->entries[i];
  }
  free(self->entries);
  free(self);
}

