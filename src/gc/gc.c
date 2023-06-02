#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "concurrency/rwulock.h"
#include "context.h"
#include "gc.h"
#include "managed_heap.h"
#include "bug.h"
#include "config.h"

#include "gc/nop/nop.h"
#include "gc/serial/serial.h"
#include "memory/heap.h"
#include "util/list_head.h"
#include "util/util.h"
#include "vec.h"

thread_local struct gc_struct* gc_current = NULL;

#define SELECT_GC_ENTRY(name, prefix, ret, func, ...) \
  case name: \
    ret = prefix ## func(__VA_ARGS__); \
    break; \

#define SELECT_GC(algo, ret, func, ...) \
  switch (algo) { \
    case GC_UNKNOWN: \
      BUG(); \
    SELECT_GC_ENTRY(GC_NOP_GC, gc_nop_, ret, func, __VA_ARGS__) \
    SELECT_GC_ENTRY(GC_SERIAL_GC, gc_serial_, ret, func, __VA_ARGS__) \
  }

struct gc_struct* gc_new(enum gc_algorithm algo, gc_flags gcFlags) {
  struct gc_struct* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  SELECT_GC(algo, self->hooks, new, gcFlags);
  if (!self->hooks || rwulock_init(&self->gcLock) < 0)
    goto failure;
  
  return self;
failure:
  gc_free(self);
  return NULL;
}

int gc_generation_count(enum gc_algorithm algo, gc_flags gcFlags) {
  int ret;
  SELECT_GC(algo, ret, generation_count, gcFlags);
  return ret;
}

void gc_start(struct gc_struct* self, struct generation* generation) {
  int genID = -1;
  if (generation)
    genID = generation->genID;
  printf("[GC at Gen%d] Start reclaiming :3\n", genID);
  size_t reclaimedBytes = 0;
  
  reclaimedBytes = self->hooks->collect(generation);
  
  printf("[GC] Heap stat: ");
  for (int i = 0; i < managed_heap_current->generationCount; i++) {
    struct generation* gen = &managed_heap_current->generations[i];
    printf("Gen%d: %10zu bytes / %10zu bytes   ", i, gen->fromHeap->usage, gen->fromHeap->size);
  }
  puts("");
  printf("[GC at Gen%d] Reclaimed %zu bytes :3\n", genID, reclaimedBytes);
}

void gc_free(struct gc_struct* self) {
  self->hooks->free(self->hooks);
  rwulock_cleanup(&self->gcLock);
  free(self);
}

bool gc_use_fast_on_gen(enum gc_algorithm algo, gc_flags gcFlags, int genID) {
  bool ret;
  SELECT_GC(algo, ret, use_fast_on_gen, gcFlags, genID);
  return ret;
}

void gc_downgrade_from_gc_mode(struct gc_struct* self) {
  rwulock_downgrade(&self->gcLock);
}

bool gc_upgrade_to_gc_mode(struct gc_struct* self) {
  return rwulock_try_upgrade(&self->gcLock);
}

void gc_for_each_root_entry(struct gc_struct* self, void (^iterator)(struct root_ref*)) {
  for (int i = 0; i < CONTEXT_STATE_COUNT; i++) {
    struct list_head* currentContext;
    list_for_each(currentContext, &managed_heap_current->contextStates[i]) {
      struct list_head* current;
      list_for_each(current, &list_entry(currentContext, struct context, list)->root)
        iterator(list_entry(current, struct root_ref, node));
    }
  }
}
