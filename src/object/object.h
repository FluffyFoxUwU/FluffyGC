#ifndef _headers_1673090887_FluffyGC_object
#define _headers_1673090887_FluffyGC_object

#include <stdatomic.h>
#include <stddef.h>

#include "object/descriptor/embedded.h"
#include "util/list_head.h"
#include "gc/gc.h"
#include "address_spaces.h"
#include "concurrency/mutex.h"
#include "concurrency/condition.h"

struct heap;
struct mutex;
struct condition;

enum reference_strength {
  REFERENCE_STRONG
};

// Use this for pointers which points to user data
// equivalent to fh_dma_ptr (the ptr must be the first entry)
struct userptr {
  void address_heap* ptr;
};

#define USERPTR(x) ((struct userptr) {x})
#define USERPTR_NULL USERPTR(NULL)

struct object_sync_structure {
  struct soc_chunk* sourceChunk;
  struct mutex lock;
  struct condition cond;
};

struct object {
  struct list_head inPromotionList;
  
  struct object_sync_structure* syncStructure;
  
  // Used during compaction phase
  // Does not need to be _Atomic because it only modified during GC
  struct object* forwardingPointer;
  
  size_t objectSize; // Represent size of data
  int age; // Number of collection survived
  
  // As long GC blocked the object is not moved
  // so this valid
  struct userptr dataPtr;
  
  struct list_head rememberedSetNode[GC_MAX_GENERATIONS];
  atomic_bool isMarked;
  
  struct {
    struct descriptor* descriptor;
    int generationID;
    
    // Some special descriptors which embedded inside object
    struct embedded_descriptor embedded;
  } movePreserve;
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

void object_init(struct object* self, struct descriptor* desc, void address_heap* data);
void object_cleanup(struct object* self, bool isDead);

struct root_ref* object_read_reference(struct object* self, size_t offset);
void object_write_reference(struct object* self, size_t offset, struct object* obj);

void object_fix_pointers(struct object* self);

// These guarantee to be safe for DMA for non reference field
// If there object_get_dma there must be corresponding object_put_dma
// the resulting userptr is not guarantee to be same
// WARNING: DO NOT trample or write or anything on reference field
struct userptr object_get_dma(struct root_ref* rootRef);
int object_put_dma(struct root_ref* rootRef, struct userptr dma);

struct object* object_resolve_forwarding(struct object* self);

struct object* object_move(struct object* self, struct heap* dest);

// Generic iterating the object for references
int object_for_each_field(struct object* self, int (^iterator)(struct object* obj, size_t offset));

int object_init_synchronization_structs(struct object* self);

#endif

