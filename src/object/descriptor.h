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

enum descriptor_unmakeable_type {
  DESCRIPTOR_UNMAKEABLE_ANY_MARKER
};

struct object_descriptor;
struct object;

struct descriptor_unmakeable_info {
  enum descriptor_unmakeable_type type;
  const char* name;
};

struct descriptor {
  enum object_type type;
  struct list_head list;
  
  union {
    struct object_descriptor* normal;
    struct descriptor_unmakeable_info* unmakeable;
  } info;
  
  // Normally always 1 because its always 
  // owner by type registry which mean
  // the descriptor may be GC-ed
  struct refcount usages;
};

struct descriptor* descriptor_new_for_object_type(struct object_descriptor* desc);
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

#endif

