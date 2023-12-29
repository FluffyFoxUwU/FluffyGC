#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>

#include "concurrency/rwlock.h"
#include "hashmap_base.h"
#include "object/descriptor.h"
#include "type_registry.h"
#include "hashmap.h"
#include "object/descriptor/object.h"
#include "object/descriptor/unmakeable.h"

#define SPECIAL_MARKERS_LIST \
  X("fox.fluffyheap.marker.Any", DESCRIPTOR_UNMAKEABLE_ANY_MARKER)

#define X(_name, enumVal) \
  UNMAKEABLE_DESCRIPTOR_DEFINE(___________marker___________info____ ## enumVal, _name, enumVal)
  SPECIAL_MARKERS_LIST
#undef X

static void* dupString(const void* a) {
  return strdup(a);
}

static size_t hashString(const void* key) {
  return hashmap_hash_string(key);
}

static int cmpString(const void* a, const void* b) {
  return strcmp(a, b);
}

struct type_registry* type_registry_new() {
  struct type_registry* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct type_registry) {};
  hashmap_init(&self->map, (void*) hashString, (void*) cmpString);
  self->map.map_base.key_free = free;
  self->map.map_base.key_dup = dupString;
  
  if (rwlock_init(&self->lock, 0) < 0)
    goto failure;
  
  #define X(name, enumVal) \
  if (type_registry_add(self, &___________marker___________info____ ## enumVal.super) < 0) \
    goto failure; \
  
  SPECIAL_MARKERS_LIST
  #undef X
  return self;

failure:
  type_registry_free(self);
  return NULL;
}

void type_registry_free(struct type_registry* self) {
  if (!self)
    return;
  
  struct descriptor* desc = NULL;
  
  hashmap_foreach_data(desc, &self->map)
    descriptor_free(desc);
  
  hashmap_cleanup(&self->map);
  free(self);
}

struct descriptor* type_registry_get_nolock(struct type_registry* self, const char* path) {
  return hashmap_get(&self->map, path);
}

int type_registry_add_nolock(struct type_registry* self, struct descriptor* new) {
  return hashmap_put(&self->map, descriptor_get_name(new), new);
}

int type_registry_remove_nolock(struct type_registry* self, struct descriptor* desc) {
  return hashmap_remove(&self->map, descriptor_get_name(desc)) == NULL ? -ENOENT : 0;
}

int type_registry_add(struct type_registry* self, struct descriptor* new) {
  rwlock_wrlock(&self->lock);
  int ret = type_registry_add_nolock(self, new);
  rwlock_unlock(&self->lock);
  return ret;
}

int type_registry_remove(struct type_registry* self, struct descriptor* desc) {
  rwlock_wrlock(&self->lock);
  int ret = type_registry_remove_nolock(self, desc);
  rwlock_unlock(&self->lock);
  return ret;
}

struct descriptor* type_registry_get(struct type_registry* self, const char* path) {
  rwlock_rdlock(&self->lock);
  struct descriptor* desc = type_registry_get_nolock(self, path);
  rwlock_unlock(&self->lock);
  return desc;
}
