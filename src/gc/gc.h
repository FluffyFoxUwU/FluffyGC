#ifndef header_1656241406_gc_h
#define header_1656241406_gc_h

#include <stdbool.h>
#include <pthread.h>

#include "../heap.h"
#include "gc_enums.h"

struct profiler;
struct gc_state {
  struct heap* heap;

  bool isGCThreadRunning;
  pthread_t gcThread;

  struct profiler* profiler;

  struct {
    struct region_reference** oldLookup;
  } fullGC;

  struct {
    float youngGCTime;
    int youngGCCount;
  } statistics;
};

struct gc_state* gc_init(struct heap* heap);
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

void gc_fix_object_refs(struct gc_state* self, struct region* sourceGen, struct object_info* ref);
void gc_fix_object_refs_custom(struct gc_state* self, struct region* sourceGen, struct object_info* ref, gc_fixer_callback fixer);

#endif




