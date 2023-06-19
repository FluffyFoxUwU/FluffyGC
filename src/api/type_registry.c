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
#include "object/object_descriptor.h"
#include "util/util.h"

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
  
  if (rwlock_init(&self->lock) < 0)
    goto failure;
  
  // Add special descriptors
  struct {
    const char* name;
    struct descriptor* desc;
  } specialTypes[] = {
    {"fox.fluffyheap.marker.Any", DESCRIPTOR_SPECIAL_ANY}
  };
  
  for (int i = 0; i < ARRAY_SIZE(specialTypes); i++)
    if (type_registry_add(self, specialTypes[i].desc->info.normal) < 0)
      goto failure;
  
  return self;

failure:
  type_registry_free(self);
  return NULL;
}

void type_registry_free(struct type_registry* self) {
  if (!self)
    return;
  
  struct object_descriptor* desc = NULL;
  
  // fox.fluffyheap.marker.* are always specially treated
  // (i.e. they are staticly defined by hand and most content of it invalid)
  hashmap_foreach_data(desc, &self->map)
    if (!util_prefixed_by("fox.fluffyheap.marker.", desc->name))
      descriptor_free(desc->parent);
  
  hashmap_cleanup(&self->map);
  free(self);
}

struct object_descriptor* type_registry_get_nolock(struct type_registry* self, const char* path) {
  return hashmap_get(&self->map, path);
}

int type_registry_add_nolock(struct type_registry* self, struct object_descriptor* new) {
  return hashmap_put(&self->map, new->name, new);
}

int type_registry_remove_nolock(struct type_registry* self, struct object_descriptor* desc) {
  return hashmap_remove(&self->map, desc->name) == NULL ? -ENOENT : 0;
}

int type_registry_add(struct type_registry* self, struct object_descriptor* new) {
  rwlock_wrlock(&self->lock);
  int ret = type_registry_add_nolock(self, new);
  rwlock_unlock(&self->lock);
  return ret;
}

int type_registry_remove(struct type_registry* self, struct object_descriptor* desc) {
  rwlock_wrlock(&self->lock);
  int ret = type_registry_remove_nolock(self, desc);
  rwlock_unlock(&self->lock);
  return ret;
}

struct object_descriptor* type_registry_get(struct type_registry* self, const char* path) {
  rwlock_rdlock(&self->lock);
  struct object_descriptor* desc = type_registry_get_nolock(self, path);
  rwlock_unlock(&self->lock);
  return desc;
}
