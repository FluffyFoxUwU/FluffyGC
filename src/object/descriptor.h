#ifndef _headers_1644388362_FoxGC_descriptors
#define _headers_1644388362_FoxGC_descriptors

#include <stdint.h>

#include "FluffyHeap/FluffyHeap.h"
#include "util/counter.h"
#include "util/list_head.h"

enum object_type {
  OBJECT_NORMAL,
  OBJECT_ARRAY,
  
  // Descriptor with this type
  // cannot be used to make new object
  // and descriptor with this type is
  // statically allocated
  OBJECT_UNMAKEABLE
};

struct object_descriptor;
struct object;
struct descriptor;

struct descriptor_ops {
  void (*postInitObject)(struct descriptor* self, struct object* obj);
  void (*runFinalizer)(struct descriptor* self, struct object* obj);
  
  // Memory given may valid until next
  // getName call of same type (even if its
  // different descriptor instance)
  const char* (*getName)(struct descriptor* self);
  
  size_t (*getObjectSize)(struct descriptor* self);
  struct descriptor* (*getDescriptorAt)(struct descriptor* self, size_t offset);
  
  // Iterator return 0 if want to continue else nonzero to stop the iterating
  // and return that value
  int (*forEachOffset)(struct descriptor* self, struct object* object, int (^iterator)(size_t offset));
  void (*free)(struct descriptor* self);
  
  bool (*isCompatible)(struct descriptor* a, struct descriptor* b);
  ssize_t (*calcOffset)(struct descriptor* self, size_t index);
};

struct descriptor {
  uint64_t foreverUniqueID;
  
  enum object_type type;
  struct list_head list;
  struct descriptor_ops* ops;
  
  // Direct usage by descriptor_acquire and descriptor_release
  // to tell GC that it still used
  struct counter directUsageCounter;
  
  struct {
    // This differ in way each descriptor assign this by themselves
    // instead by codes at src/api/*.c
    fh_type_info typeInfo;
  } api;
};

const char* descriptor_object_type_tostring(enum object_type type);

int descriptor_init(struct descriptor* self, enum object_type type, struct descriptor_ops* ops, fh_type_info* typeInfo);
void descriptor_free(struct descriptor* self);

int descriptor_for_each_offset(struct object* self, int (^iterator)(size_t offset));
int descriptor_is_assignable_to(struct object* self, size_t offset, struct descriptor* b);

void descriptor_init_object(struct descriptor* self, struct object* obj);

size_t descriptor_get_object_size(struct descriptor* self);

// Memory valid until next call even if different descriptor
const char* descriptor_get_name(struct descriptor* self);

// Only track number of keepalive uses, does not automaticly
// releases as there might living object using it
void descriptor_acquire(struct descriptor* self);
void descriptor_release(struct descriptor* self);

void descriptor_run_finalizer_on(struct descriptor* self, struct object* obj);

// For object type `index` is index to which field not offset being
// passthrough as it is
ssize_t descriptor_calc_offset(struct descriptor* self, size_t index);

#endif

