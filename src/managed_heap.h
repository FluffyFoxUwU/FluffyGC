#ifndef _headers_1674646850_FluffyGC_managed_heap
#define _headers_1674646850_FluffyGC_managed_heap

// The global state
// Tracking various things

#include <pthread.h>

#include "list.h"
#include "vec.h"

struct context;
struct heap;

#define MANAGED_HEAP_NUM_GENERATION 2

struct managed_heap {
  pthread_rwlock_t gcLock;
  
  struct heap* generations[MANAGED_HEAP_NUM_GENERATION];
  list_t* rememberedSets[MANAGED_HEAP_NUM_GENERATION];
  
  vec_t(struct context*) threads;
};

// Must be called from user/mutator context
// with no lock being held. this function may block
// Please call periodicly if GC being blocked
// for long time
void managed_heap_gc_safepoint();

// Cannot be nested
// consider using context_(un)block_gc functions instead
void managed_heap_block_gc(struct managed_heap* self);
void managed_heap_unblock_gc(struct managed_heap* self);

#endif

