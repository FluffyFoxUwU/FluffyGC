#ifndef _headers_1674646850_FluffyGC_managed_heap
#define _headers_1674646850_FluffyGC_managed_heap

// The global state
// Tracking various things

#include "list.h"
#include "concurrency/rwlock.h"
#include "util/list_head.h"
#include "vec.h"

struct context;
struct heap;
struct gc_struct;
struct object;
enum gc_algorithm;

struct generation {
  struct heap* fromHeap;
  struct heap* toHeap;
  struct list_head rememberedSet;
};

struct managed_heap {
  struct rwlock gcLock;
  struct gc_struct* gcState;
  
  vec_t(struct context*) threads;
  
  int generationCount;
  struct generation generations[];
};

struct managed_heap* managed_heap_new(enum gc_algorithm algo, int generationCount, size_t* generationSizes, int gcFlags);
void managed_heap_free(struct managed_heap* self);

// Must be called from user/mutator context
// with no lock being held. this function may block
// Please call periodicly if GC being blocked
// for long time
void managed_heap__gc_safepoint();

// Cannot be nested
// consider using context_(un)block_gc functions instead
void managed_heap__block_gc(struct managed_heap* self);
void managed_heap__unblock_gc(struct managed_heap* self);

#define managed_heap_gc_safepoint() do { \
  atomic_thread_fence(memory_order_acquire); \
  managed_heap__gc_safepoint((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#define managed_heap_block_gc(self) do { \
  atomic_thread_fence(memory_order_acquire); \
  managed_heap__block_gc((self)); \
} while (0)

#define managed_heap_unblock_gc(self) do { \
  managed_heap__unblock_gc((self)); \
  atomic_thread_fence(memory_order_release); \
} while (0)

#endif

