#ifndef _headers_1644388362_FoxGC_descriptors
#define _headers_1644388362_FoxGC_descriptors

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

#include "object.h"
#include "refcount.h"

struct region;
struct object;

struct descriptor_field {
  const char* name;
  size_t offset;

  enum object_type dataType;
  enum reference_strength strength;
};

struct descriptor_typeid {
  const char* name;
  
  uintptr_t ownerID;
  uintptr_t typeID;
};

typedef struct descriptor {
  struct descriptor_typeid id;

  size_t objectSize;
  
  struct refcount refcount;
  
  int numFields;
  struct descriptor_field fields[];
} descriptor_t;

struct descriptor* descriptor_new(struct descriptor_typeid id, size_t objectSize, int numPointers, struct descriptor_field* fields);

void descriptor_init(struct descriptor* self, struct object* obj);

int descriptor_get_index_from_offset(struct descriptor* self, size_t offset);

void descriptor_write_ptr(struct descriptor* self, struct object* data, int index, struct object* ptr);
struct object* descriptor_read_ptr(struct descriptor* self, struct object* data, int index);

void descriptor_acquire(struct descriptor* self);
void descriptor_release(struct descriptor* self);

#endif

