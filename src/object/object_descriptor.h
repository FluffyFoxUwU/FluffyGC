#ifndef _headers_1686991480_FluffyGC_object_descriptor
#define _headers_1686991480_FluffyGC_object_descriptor

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stddef.h>

#include "FluffyHeap.h"
#include "util/list_head.h"
#include "vec.h"
#include "object.h"

// This specifically only handles OBJECT_NORMAL type
// not arrays

struct object_descriptor;
struct descriptor;
struct object;

struct object_descriptor_field {
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

struct object_descriptor {
  struct descriptor* parent;
  
  const char* name;
  bool isDefined;

  size_t objectSize;
  size_t alignment;
  
  struct {
    struct list_head list;
    fh_descriptor_param param;
  } api;
  
  vec_t(struct object_descriptor_field) fields;
};

struct object_descriptor* object_descriptor_new();

void object_descriptor_init(struct object_descriptor* self);
void object_descriptor_init_object(struct object_descriptor* self, struct object* obj);
int object_descriptor_get_index_from_offset(struct object_descriptor* self, size_t offset);
void object_descriptor_free(struct object_descriptor* self);
void object_descriptor_for_each_offset(struct object_descriptor* self, struct object* obj, void (^iterator)(size_t offset));
struct descriptor* object_descriptor_get_at(struct object_descriptor* self, size_t offset);

size_t object_descriptor_get_object_size(struct object_descriptor* self);
size_t object_descriptor_get_alignment(struct object_descriptor* self);
const char* object_descriptor_get_name(struct object_descriptor* self);

#endif

