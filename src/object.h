#ifndef _headers_1673090887_FluffyGC_object
#define _headers_1673090887_FluffyGC_object

#include <stddef.h>

enum object_type {
  OBJECT_NORMAL,
  OBJECT_OPAQUE
};

enum reference_strength {
  REFERENCE_STRONG
};

struct object {
  // Used during compaction phase
  struct object* forwardingPointer;
  
  struct descriptor* descriptor;
  size_t objectSize; // Represent size of data in payload (including redzones)
  int age; // Number of collection survived
  
  char payload[];
};

typedef _Atomic(struct object*) object_ptr_atomic;

/*
Declaration example

struct node {
  void* someNativeData;
  
  // GC managed pointer shall be atomic
  // and must not accessed directly
  _Atomic(struct node*) next;
  _Atomic(struct node*) prev;
};
*/

extern const void* const object_failure_ptr;
#define OBJECT_FAILURE_PTR ((void*) object_failure_ptr)

// object_(read/write)_ptr are safe without DMA
// Return OBJECT_FAILURE_PTR on failure
[[nodiscard]]
struct object* object_read_ptr(struct object* self, size_t offset);
void object_write_ptr(struct object* self, size_t offset, struct object* obj);

void* object_get_dma(struct object* self);
void object_put_dma(struct object* self, void* dma);

struct object* object_resolve_forwarding(struct object* self);

#endif

