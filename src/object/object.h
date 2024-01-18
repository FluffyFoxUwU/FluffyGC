#ifndef _headers_1673090887_FluffyGC_object
#define _headers_1673090887_FluffyGC_object

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include "object/descriptor.h"
#include "object/descriptor/embedded.h"
#include "util/list_head.h"
#include "gc/gc.h"
#include "concurrency/mutex.h"
#include "concurrency/condition.h"

struct heap;
struct heap_block;
struct mutex;
struct condition;

enum reference_strength {
  REF_STRONG,
  REF_SOFT,
  REF_WEAK,
  REF_PHANTOM
};

const char* object_ref_strength_tostring(enum reference_strength strength);

struct object_sync_structure {
  struct soc_chunk* sourceChunk;
  struct mutex lock;
  struct condition cond;
};

struct object_override_info {
  void* overridePtr;
};

struct object {
  struct list_head inPromotionList;
  
  // Used during compaction phase
  // Does not need to be _Atomic because it only modified during GC
  struct object* forwardingPointer;
  
  size_t objectSize; // Represent size of data
  int age; // Number of collection survived
   
  struct list_head rememberedSetNode[GC_MAX_GENERATIONS];
  atomic_bool isMarked;

  // This will be copied over don't store
  // something which cant be moved
  struct {
    struct object_sync_structure* syncStructure;
  
    struct descriptor* descriptor;
    int generationID;
    uint64_t foreverUniqueID;

    // NULL if not overriden
    struct object_override_info* overridePtr;
    
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

void object_init(struct object* self, struct descriptor* desc);
void object_cleanup(struct object* self, bool canRunFinalizer);

struct root_ref* object_read_reference(struct object* self, size_t offset);
void object_write_reference(struct object* self, size_t offset, struct object* obj);

void object_fix_pointers(struct object* self);
struct heap_block* object_get_heap_block(struct object* self);

// GC must be blocked as object may be moved
// Get read/write pointer which may not
// be equal to heap pointer
void* object_get_ptr(struct object* self);

// Mark the start and end of usage of
// ptr must be called before object_get_ptr
void object_ptr_use_start(struct object* self);
void object_ptr_use_end(struct object* self);

// Blocks new object_get_ptr on self
// and wait until remaining to be gone
void object_block_ptr(struct object* self);
void object_unblock_ptr(struct object* self);

// Must be called when ptr access is blocked
// returns old ptr
void object_set_override_ptr(struct object* self, struct object_override_info* newPtr);
struct object_override_info* object_get_override_ptr(struct object* self);

// Always return pointer to heap which
// may change if object moved
void* object_get_backing_ptr(struct object* self);

struct object* object_resolve_forwarding(struct object* self);
struct object* object_move(struct object* self, struct heap* dest);

// Generic iterating the object for references
int object_for_each_field(struct object* self, int (^iterator)(struct object* obj, size_t offset));

int object_init_synchronization_structs(struct object* self);

// Uses internal static buffer
const char* object_get_unique_name(struct object* self);;

#endif

