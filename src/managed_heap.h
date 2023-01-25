#ifndef _headers_1674646850_FluffyGC_managed_heap
#define _headers_1674646850_FluffyGC_managed_heap

// The global state
// Tracking various things

#include "vec.h"

struct context;
struct heap;

struct managed_heap {
  struct heap* youngHeap;
  struct heap* oldHeap;
  
  vec_t(struct context*) threads;
};

// Must be called from user/mutator context
// with no lock being held. this function may block
// Please call periodicly if GC being blocked
// for long time
void managed_heap_gc_safepoint();

#endif

