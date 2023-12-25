#ifndef _headers_1674646850_FluffyGC_managed_heap
#define _headers_1674646850_FluffyGC_managed_heap

// The global state
// Tracking various things

#include <stdint.h>
#include <threads.h>

#include "FluffyHeap/FluffyHeap.h"
#include "concurrency/completion.h"
#include "concurrency/mutex.h"
#include "gc/gc.h"
#include "util/id_generator.h"
#include "util/list_head.h"
#include "gc/gc_flags.h"
#include "context.h"

struct descriptor;
struct context;
struct heap;
struct gc_struct;
struct object;
struct api_state;
enum gc_algorithm;

struct generation_params {
  size_t size;
  size_t earlyPromoteSize;
  int promotionAge;
  
  // Young uses
  // 1. Only atomic pointer (no freelist traversal)
  // 2. Semispace heap scheme
  // 3. No compaction phase
  // Old uses
  // 1. Both atomic and freelist used
  // 2. Has compaction phase
};

struct generation {
  struct heap* fromHeap;
  struct heap* toHeap;
  struct generation_params param;
  int genID;
  
  struct mutex rememberedSetLock;
  struct list_head rememberedSet;
  bool useFastOnly;
};

struct managed_heap {
  struct gc_struct* gcState;
  uint64_t foreverUniqueID;
  
  struct {
    struct id_generator_state object;
    struct id_generator_state context;
    struct id_generator_state dmaPtr;
    struct id_generator_state descriptor;
  } idGenerators;
  
  struct {
    struct mutex descriptorLoaderSerializationLock;
    void* udata;
    fh_descriptor_loader descriptorLoader;
    
    struct type_registry* registry;
    struct api_state* state;
  } api;
  
  struct list_head descriptorList;
  
  struct completion gcCompleted;
  
  struct mutex contextTrackerLock;
  int64_t contextCount;
  struct list_head contextStates[CONTEXT_STATE_COUNT];
  
  int generationCount;
  struct generation generations[];
};

extern thread_local struct managed_heap* managed_heap_current;

struct managed_heap* managed_heap_new(enum gc_algorithm algo, int genCount, struct generation_params* generationParams, gc_flags gcFlags);
void managed_heap_free(struct managed_heap* self);

struct root_ref* managed_heap_alloc_object(struct descriptor* desc);

int managed_heap_attach_thread(struct managed_heap* self);
void managed_heap_detach_thread(struct managed_heap* self);

struct context* managed_heap_new_context(struct managed_heap* self);
void managed_heap_free_context(struct managed_heap* self, struct context* ctx);

// Context swapping
int managed_heap_swap_context(struct context* new);
void managed_heap_set_context_state(struct context* ctx, enum context_state state);

// ID generators
uint64_t managed_heap_generate_object_id();
uint64_t managed_heap_generate_context_id();
uint64_t managed_heap_generate_dma_ptr_id();
uint64_t managed_heap_generate_descriptor_id();

#define managed_heap_set_states(self, ctx) \
  context_current = (ctx); \
  gc_current = (self) ? ((struct managed_heap*) self)->gcState : NULL; \
  managed_heap_current = (self); \

// These two context create new scope and MUST
// NOT jump/goto to outside of the scope 
#define managed_heap_push_states(self, ctx) do { \
  struct context* ______prevContext = context_current; \
  struct gc_struct* ______prevGC = gc_current; \
  struct managed_heap* ______prevManangedHeap = managed_heap_current; \
  managed_heap_set_states((self), (ctx)); \
  do {} while (0)

#define managed_heap_pop_states() \
    managed_heap_current = ______prevManangedHeap; \
    gc_current = ______prevGC; \
    context_current = ______prevContext; \
  } while(0)

#endif

