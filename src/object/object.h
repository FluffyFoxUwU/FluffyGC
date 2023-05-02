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

// Use this for pointers which points to user data
struct userptr {
  void* ptr;
};
#define USERPTR(x) ((struct userptr) {x})
#define USERPTR_NULL USERPTR(NULL)

struct object {
  // Used during compaction phase
  // Does not need to be _Atomic because it only modified during GC
  struct object* forwardingPointer;
  
  struct descriptor* descriptor;
  size_t objectSize; // Represent size of data
  int age; // Number of collection survived
  
  struct userptr dataPtr;
};

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

void object_init(struct object* self, struct descriptor* desc, void* data);

// object_(read/write)_ptr are safe without DMA
[[nodiscard]]
struct root_ref* object_read_reference(struct object* self, size_t offset);
void object_write_reference(struct object* self, size_t offset, struct object* obj);

// These guarantee to be safe for DMA for non object field
// If there object_get_dma there must be corresponding object_put_dma
// the resulting userptr is not guarantee to be same
struct userptr object_get_dma(struct root_ref* rootRef);
int object_put_dma(struct root_ref* rootRef, struct userptr dma);

struct object* object_resolve_forwarding(struct object* self);

#endif

