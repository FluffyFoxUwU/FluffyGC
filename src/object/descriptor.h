#ifndef _headers_1644388362_FoxGC_descriptors
#define _headers_1644388362_FoxGC_descriptors

#include <stddef.h>

#include "util/refcount.h"
#include "util/list_head.h"

enum object_type {
  OBJECT_NORMAL,
  
  // Descriptor with this type
  // cannot be used to make new object
  // and descriptor with this type is
  // statically allocated
  OBJECT_UNMAKEABLE_STATIC
};

struct object_descriptor;
struct object;
struct descriptor;

struct descriptor_ops {
  void (*runFinalizer)(struct descriptor* self, struct object* obj);
  const char* (*getName)(struct descriptor* self);
  size_t (*getAlignment)(struct descriptor* self);
  size_t (*getObjectSize)(struct descriptor* self);
  void (*initObject)(struct descriptor* self, struct object* obj);
  struct descriptor* (*getDescriptorAt)(struct descriptor* self, size_t offset);
  void (*forEachOffset)(struct descriptor* self, struct object* object, void (^iterator)(size_t offset));
  void (*free)(struct descriptor* self);
};

struct descriptor {
  enum object_type type;
  struct list_head list;
  struct descriptor_ops* ops;
  
  // Normally always 1 because its always 
  // owner by type registry which mean
  // the descriptor may be GC-ed
  struct refcount usages;
};

int descriptor_init(struct descriptor* self, enum object_type type, struct descriptor_ops* ops);
void descriptor_free(struct descriptor* self);

void descriptor_for_each_offset(struct object* self, void (^iterator)(size_t offset));
int descriptor_is_assignable_to(struct object* self, size_t offset, struct descriptor* b);

void descriptor_init_object(struct descriptor* self, struct object* obj);

size_t descriptor_get_object_size(struct descriptor* self);
size_t descriptor_get_alignment(struct descriptor* self);
const char* descriptor_get_name(struct descriptor* self);

// Only track number of uses, does not automaticly
// releases as there might living object using it
void descriptor_acquire(struct descriptor* self);
void descriptor_release(struct descriptor* self);

void descriptor_run_finalizer_on(struct descriptor* self, struct object* obj);

#endif

