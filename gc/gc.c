#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>

#include <flup/bug.h>
#include <flup/container_of.h>
#include <flup/data_structs/list_head.h>
#include <flup/core/panic.h>
#include <flup/core/logger.h>
#include <flup/data_structs/dyn_array.h>
#include <flup/data_structs/buffer.h>

#include "heap/heap.h"
#include "memory/arena.h"
#include "heap/generation.h"

#include "gc.h"

void gc_on_allocate(struct arena_block* block, struct generation* gen) {
  block->gcMetadata = (struct gc_block_metadata) {
    .markBit = !gen->gcState->mutatorMarkedBitValue,
    .owningGeneration = gen
  };
}

void gc_on_reference_lost(struct arena_block* objectWhichIsGoingToBeOverwritten) {
  struct gc_block_metadata* metadata = &objectWhichIsGoingToBeOverwritten->gcMetadata;
  struct gc_per_generation_state* gcState = metadata->owningGeneration->gcState;
  
  // Doesn't need to enqueue to mark queue as its
  // purpose is for queuing unmarked objects when GC running
  if (!gcState->cycleInProgress)
    return;
  
  bool prevMarkBit = atomic_exchange(&metadata->markBit, gcState->GCMarkedBitValue);
  if (prevMarkBit == gcState->GCMarkedBitValue)
    return;
  
  // Enqueue an pointer
  flup_buffer_write_no_fail(gcState->mutatorMarkQueue, &objectWhichIsGoingToBeOverwritten, sizeof(void*));
}

struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen) {
  struct gc_per_generation_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_per_generation_state) {
    .ownerGen = gen
  };
  
  if (!(self->snapshotOfRootSet = flup_dyn_array_new(sizeof(void*), 0)))
    goto failure;
  if (!(self->mutatorMarkQueue = flup_buffer_new(GC_MARK_QUEUE_SIZE)))
    goto failure;
  return self;

failure:
  gc_per_generation_state_free(self);
  return NULL;
}

void gc_per_generation_state_free(struct gc_per_generation_state* self) {
  if (!self)
    return;
  
  flup_dyn_array_free(self->snapshotOfRootSet);
  flup_buffer_free(self->mutatorMarkQueue);
  free(self);
}

static void doMark(struct gc_per_generation_state* state, struct arena_block* block) {
  block->gcMetadata.markBit = state->GCMarkedBitValue;
}

struct cycle_state {
  struct gc_per_generation_state* self;
  struct arena* arena;
  struct heap* heap;
};

// Must be STW once GC has own thread
static void takeRootSnapshotPhase(struct cycle_state* state) {
  int ret;
  if ((ret = flup_dyn_array_reserve(state->self->snapshotOfRootSet, state->heap->rootEntryCount)) < 0)
    flup_panic("Error reserving memory for root set snapshot: flup_dyn_array_reserve: %d", ret);
  
  flup_list_head* current;
  flup_list_for_each(&state->heap->root, current) {
    struct root_ref* ref = container_of(current, struct root_ref, node);
    if (flup_dyn_array_append(state->self->snapshotOfRootSet, &ref) < 0)
      flup_panic("This can't occur TwT");
  }
}

static void markingPhase(struct cycle_state* state) {
  for (size_t i = 0; i < state->self->snapshotOfRootSet->length; i++) {
    struct root_ref** ref;
    int ret = flup_dyn_array_get(state->self->snapshotOfRootSet, i, (void**) &ref);
    BUG_ON(ret < 0);
    doMark(state->self, (*ref)->obj);
  }
}

static void processMutatorMarkQueuePhase(struct cycle_state* state) {
  struct arena_block* current;
  int ret;
  while ((ret = flup_buffer_read2(state->self->mutatorMarkQueue, &current, sizeof(current), FLUP_BUFFER_READ2_DONT_WAIT_FOR_DATA)) >= 0)
    doMark(state->self, current);
  BUG_ON(ret == -EMSGSIZE);
}

static void sweepPhase(struct cycle_state* state) {
  size_t count = state->self->ownerGen->arena->blocks->length;
  size_t sweepedCount = 0;
  for (size_t i = 0; i < count; i++) {
    struct arena_block** blockPtr;
    int ret = flup_dyn_array_get(state->arena->blocks, i, (void**) &blockPtr);
    BUG_ON(ret < 0);
    
    struct arena_block* block = *blockPtr;
    if (!block->used)
      continue;
    
    // Object is alive continuing
    if (block->gcMetadata.markBit == state->self->GCMarkedBitValue)
      continue;
    
    sweepedCount++;
    arena_dealloc(state->arena, block);
  }
  pr_info("Sweeped %zu out of %zu objects", sweepedCount, count);
}

void gc_start_cycle(struct gc_per_generation_state* self) {
  struct arena* arena = self->ownerGen->arena;
  struct heap* heap = self->ownerGen->ownerHeap;
  pr_info("Before cycle mem usage: %f MiB", (float) arena->currentUsage / 1024.0f / 1024.0f);
  
  struct cycle_state state = {
    .arena = arena,
    .self = self,
    .heap = heap
  };
  
  struct timespec start, end;
  struct timespec start2, end2;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
  self->mutatorMarkedBitValue = !self->mutatorMarkedBitValue;
  self->cycleInProgress = true;
  
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start2);
  takeRootSnapshotPhase(&state);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end2);
  double duration2 =
    ((double) end2.tv_sec + ((double) end2.tv_nsec) / 1'000'000'000.0f) -
    ((double) start2.tv_sec + ((double) start2.tv_nsec) / 1'000'000'000.0f);
  pr_info("Taking root snapshot was : %lf ms", duration2 * 1000.f);
  
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start2);
  markingPhase(&state);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end2);
  duration2 =
    ((double) end2.tv_sec + ((double) end2.tv_nsec) / 1'000'000'000.0f) -
    ((double) start2.tv_sec + ((double) start2.tv_nsec) / 1'000'000'000.0f);
  pr_info("Marking time was         : %lf ms", duration2 * 1000.f);
  
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start2);
  processMutatorMarkQueuePhase(&state);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end2);
  duration2 =
    ((double) end2.tv_sec + ((double) end2.tv_nsec) / 1'000'000'000.0f) -
    ((double) start2.tv_sec + ((double) start2.tv_nsec) / 1'000'000'000.0f);
  pr_info("Finalizing mark time was : %lf ms", duration2 * 1000.f);
  
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start2);
  sweepPhase(&state);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end2);
  duration2 =
    ((double) end2.tv_sec + ((double) end2.tv_nsec) / 1'000'000'000.0f) -
    ((double) start2.tv_sec + ((double) start2.tv_nsec) / 1'000'000'000.0f);
  pr_info("Sweep time was           : %lf ms", duration2 * 1000.f);
  
  self->GCMarkedBitValue = !self->GCMarkedBitValue;
  self->cycleInProgress = false;
  flup_dyn_array_remove(self->snapshotOfRootSet, 0, self->snapshotOfRootSet->length);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
  
  double duration =
    ((double) end.tv_sec + ((double) end.tv_nsec) / 1'000'000'000.0f) -
    ((double) start.tv_sec + ((double) start.tv_nsec) / 1'000'000'000.0f);
  pr_info("Cycle time was: %lf ms", duration * 1000.f);
  pr_info("After cycle mem usage: %f MiB", (float) arena->currentUsage / 1024.0f / 1024.0f);
}


