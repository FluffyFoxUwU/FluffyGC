#include <stdlib.h>
#include <assert.h>

#include "region.h"
#include "root.h"
#include "config.h"

struct root* root_new(int size) {
  struct root* self = malloc(sizeof(*self));
  if (!self)
    goto failure;
  
  self->size = size;
  self->usage = 0;

  self->entries = malloc(size * sizeof(*self->entries));
  if (!self->entries)
    goto failure;
  
  for (int i = 0; i < size; i++) {
    if (IS_ENABLED(CONFIG_DEBUG_DONT_REUSE_ROOT_REFERENCE))
      self->entries[i].refToSelf = NULL;
    else
      self->entries[i].refToSelf = &self->entries[i];
    
    self->entries[i].isValid = false;
    self->entries[i].owner = self;
    self->entries[i].index = i;
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

  struct root_reference* rootRef;
  if (IS_ENABLED(CONFIG_DEBUG_DONT_REUSE_ROOT_REFERENCE))
    rootRef = malloc(sizeof(*rootRef));
  else
    rootRef = &self->entries[freePos];
  
  self->usage++;
  
  rootRef->isWeak = false;
  rootRef->isValid = true;
  rootRef->data = ref;
  rootRef->index = freePos;
  rootRef->refToSelf = rootRef;
  rootRef->owner = self;
  
  if (IS_ENABLED(CONFIG_DEBUG_DONT_REUSE_ROOT_REFERENCE)) {
    self->entries[freePos].refToSelf = rootRef;
    self->entries[freePos].isValid = true;
  }

  return rootRef;
}

static void remove_entry(struct root* self, struct root_reference* ref){
  assert(self == ref->owner);

  self->usage--;
  ref->isValid = false;
  ref->data = NULL;

  if (IS_ENABLED(CONFIG_DEBUG_DONT_REUSE_ROOT_REFERENCE)) {
    ref->refToSelf = NULL;
    
    self->entries[ref->index] = *ref;
    free(ref);
  }
}

void root_remove(struct root* self, struct root_reference* ref) {
  remove_entry(self, ref);
}

void root_clear(struct root* self) {
  for (int i = 0; i < self->size; i++)
    if (self->entries[i].refToSelf)
      root_remove(self, self->entries[i].refToSelf);
}

void root_free(struct root* self) {
  if (!self)
    return;
  
  root_clear(self);
  free(self->entries);
  free(self);
}

