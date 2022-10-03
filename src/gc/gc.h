#ifndef header_1656241406_gc_h
#define header_1656241406_gc_h

#include <stdbool.h>
#include <pthread.h>

#include "../heap.h"
#include "gc_enums.h"

struct profiler;
struct gc_state {
  struct heap* heap;

  // Workers
  struct thread_pool* workerPool;
  
  pthread_t gcThread;

  bool isExplicit;
  
  pthread_barrier_t gcThreadReadyBarrier;
  struct profiler* profiler;

  struct {
    struct region_reference** oldLookup;
  } fullGC;

  struct {
    float youngGCTime;
    int youngGCCount;
  } statistics;
  
  bool isGCThreadReadyBarrierInited;
  bool isGCThreadRunning;
};

struct gc_state* gc_init(struct heap* heap, int workerCount);
void gc_cleanup(struct gc_state* self);

// Fix roots
void gc_fix_root(struct gc_state* self);

void gc_clear_old_to_young_card_table(struct gc_state* self);
void gc_clear_young_to_old_card_table(struct gc_state* self);

// Triggers
void gc_trigger_young_collection(struct gc_state* self, enum report_type reason);
void gc_trigger_old_collection(struct gc_state* self, enum report_type reason);
void gc_trigger_full_collection(struct gc_state* self, enum report_type reason, bool isExplicit);

// Fixers
typedef void* (^gc_fixer_callback)(void* ptr);

void gc_fix_object_refs(struct gc_state* self, struct object_info* ref);
void gc_fix_object_refs_custom(struct gc_state* self, struct object_info* ref, gc_fixer_callback fixer);

// Clear weak/soft ref contained
// in the object
//
// Should be called this before 
// sweep but after marking phase
void gc_clear_weak_refs(struct gc_state* state, struct object_info* object);
void gc_clear_soft_refs(struct gc_state* state, struct object_info* object);

#endif




