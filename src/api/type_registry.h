#ifndef _headers_1686829262_FluffyGC_descriptor_registry
#define _headers_1686829262_FluffyGC_descriptor_registry

// Registry for various type (Not arrays)

#include "concurrency/rwlock.h"
#include "hashmap.h"

struct type_registry {
  struct rwlock lock;
  HASHMAP(char, struct object_descriptor) map;
};

struct type_registry* type_registry_new();
void type_registry_free(struct type_registry* self);

struct object_descriptor* type_registry_get(struct type_registry* self, const char* path);
struct object_descriptor* type_registry_get_nolock(struct type_registry* self, const char* path);

int type_registry_add(struct type_registry* self, struct object_descriptor* new);
int type_registry_add_nolock(struct type_registry* self, struct object_descriptor* new);
int type_registry_remove(struct type_registry* self, struct object_descriptor* desc);
int type_registry_remove_nolock(struct type_registry* self, struct object_descriptor* desc);

#endif

