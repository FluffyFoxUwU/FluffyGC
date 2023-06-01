#ifndef _headers_1644388362_FoxGC_descriptors
#define _headers_1644388362_FoxGC_descriptors

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>

#include "vec.h"
#include "object.h"
#include "util/refcount.h"

struct descriptor;
struct object;

struct descriptor_field {
  const char* name;
  size_t offset;
  
  struct descriptor* dataType;
  enum reference_strength strength;
};

#define DESCRIPTOR_FIELD(type, member, _dataType, refStrength) { \
  .name = #member, \
  .offset = offsetof(type, member), \
  .dataType = (_dataType), \
  .strength = (refStrength) \
}
#define DESCRIPTOR_FIELD_END() {.name = NULL}

struct descriptor_typeid {
  const char* name;
  
  uintptr_t ownerID;
  uintptr_t typeID;
};

struct descriptor {
  struct descriptor_typeid id;
  enum object_type objectType;
  bool isDefined;

  size_t objectSize;
  size_t alignment;
  
  struct refcount refcount;
  
  vec_t(struct descriptor_field) fields;
};

struct descriptor_type {
  struct descriptor_typeid id;
  size_t alignment;
  size_t objectSize;
  enum object_type objectType;
  struct descriptor_field* fields;
};

struct descriptor* descriptor_new();
int descriptor_define(struct descriptor* self, struct descriptor_type* type);

void descriptor_init_object(struct descriptor* self, struct object* obj);

int descriptor_get_index_from_offset(struct descriptor* self, size_t offset);

void descriptor_acquire(struct descriptor* self);
void descriptor_release(struct descriptor* self);

void descriptor_for_each_field(struct object* self, void (^iterator)(struct object* obj,size_t offset));

// Different object type is not compatible
// Incompatible cases
// TypeA    ->  TypeA[]     is not compatible
// TypeA    ->  TypeB       is not compatible
// TypeA[]  ->  TypeB[]     is not compatible
// Compatible cases
// TypeA    ->  TypeA       is compatible
// TypeA[]  ->  TypeA[]     is compatible
// Return bool on sucess and -errno on error
int descriptor_is_assignable_to(struct descriptor* self, size_t offset, struct descriptor* desc);

#endif

