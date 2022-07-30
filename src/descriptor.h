#ifndef _headers_1644388362_FoxGC_descriptors
#define _headers_1644388362_FoxGC_descriptors

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>

#include "heap.h"

struct region;
struct region_reference;

enum field_type {
  DESCRIPTOR_FIELD_TYPE_UNKNOWN,
  DESCRIPTOR_FIELD_TYPE_STRONG,
  DESCRIPTOR_FIELD_TYPE_WEAK
};

struct descriptor_field {
  const char* name;
  size_t offset;

  enum object_type dataType;
  enum field_type type;
};

struct descriptor_typeid {
  const char* name;
  
  uintptr_t ownerID;
  uintptr_t typeID;
};

typedef struct descriptor {
  struct descriptor_typeid id;

  size_t objectSize;
  
  atomic_bool alreadyUnregistered;
  atomic_int counter;
  
  int numFields;
  struct descriptor_field fields[];
} descriptor_t;

struct descriptor* descriptor_new(struct descriptor_typeid id, size_t objectSize, int numPointers, struct descriptor_field* fields);

void descriptor_init(struct descriptor* self, struct region* region, struct region_reference* regionRef);

int descriptor_get_index_from_offset(struct descriptor* self, size_t offset);

void descriptor_write_ptr(struct descriptor* self, struct region* region, struct region_reference* data, int index, void* ptr);
void* descriptor_read_ptr(struct descriptor* self, struct region* region, struct region_reference* data, int index);

void descriptor_acquire(struct descriptor* self);

// True if its free'd due counter reached 0
bool descriptor_release(struct descriptor* self);

#endif

