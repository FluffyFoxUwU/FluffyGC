#define _GNU_SOURCE
#include <pthread.h>

#include <errno.h>
#include <stddef.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <time.h>

#include <flup/bug.h>
#include <flup/thread/thread.h>
#include <flup/concurrency/cond.h>
#include <flup/concurrency/mutex.h>
#include <flup/concurrency/rwlock.h>
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
  block->gcMetadata.markBit = !atomic_load(&gen->gcState->mutatorMarkedBitValue);
  block->gcMetadata.owningGeneration = gen;
  
  atomic_store(&block->gcMetadata.isValid, true);
  
  size_t usage = atomic_load(&gen->arena->currentUsage);
  size_t maxSize = gen->arena->maxSize;
  size_t softLimit = (size_t) ((float) maxSize * 0.4f);
  
  // Start GC cycle so memory freed before mutator has to start
  // waiting on GC 
  if (usage > softLimit) {
    gc_start_cycle_async(gen->gcState);
  }
}

void gc_on_reference_lost(struct arena_block* objectWhichIsGoingToBeOverwritten) {
  struct gc_block_metadata* metadata = &objectWhichIsGoingToBeOverwritten->gcMetadata;
  struct gc_per_generation_state* gcState = metadata->owningGeneration->gcState;
  
  // Doesn't need to enqueue to mark queue as its
  // purpose is for queuing unmarked objects when GC running
  if (!atomic_load(&gcState->cycleInProgress))
    return;
  
  bool prevMarkBit = atomic_exchange(&metadata->markBit, gcState->GCMarkedBitValue);
  if (prevMarkBit == gcState->GCMarkedBitValue)
    return;
  
  // Enqueue an pointer
  flup_buffer_write_no_fail(gcState->mutatorMarkQueue, &objectWhichIsGoingToBeOverwritten, sizeof(void*));
}

static void gcThread(void* _self);
struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen) {
  struct gc_per_generation_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_per_generation_state) {
    .ownerGen = gen
  };
  
  if (!(self->gcLock = flup_rwlock_new()))
    goto failure;
  if (!(self->snapshotOfRootSet = flup_dyn_array_new(sizeof(void*), 0)))
    goto failure;
  if (!(self->mutatorMarkQueue = flup_buffer_new(GC_MARK_QUEUE_SIZE)))
    goto failure;
  if (!(self->invokeCycleLock = flup_mutex_new()))
    goto failure;
  if (!(self->invokeCycleDoneEvent = flup_cond_new()))
    goto failure;
  if (!(self->gcRequestLock = flup_mutex_new()))
    goto failure;
  if (!(self->gcRequestedCond = flup_cond_new()))
    goto failure;
  if (!(self->statsLock = flup_mutex_new()))
    goto failure;
  if (!(self->thread = flup_thread_new(gcThread, self)))
    goto failure;
  return self;

failure:
  gc_per_generation_state_free(self);
  return NULL;
}

static void callGCAsync(struct gc_per_generation_state* self, enum gc_request request) {
  flup_mutex_lock(self->gcRequestLock);
  self->gcRequest = request;
  flup_mutex_unlock(self->gcRequestLock);
  flup_cond_wake_one(self->gcRequestedCond);
}

void gc_per_generation_state_free(struct gc_per_generation_state* self) {
  if (!self)
    return;
  
  if (self->thread) {
    callGCAsync(self, GC_SHUTDOWN);
    flup_thread_wait(self->thread);
    flup_thread_free(self->thread);
  }
  flup_mutex_free(self->statsLock);
  flup_cond_free(self->gcRequestedCond);
  flup_mutex_free(self->gcRequestLock);
  flup_cond_free(self->invokeCycleDoneEvent);
  flup_mutex_free(self->invokeCycleLock);
  flup_rwlock_free(self->gcLock);
  flup_dyn_array_free(self->snapshotOfRootSet);
  flup_buffer_free(self->mutatorMarkQueue);
  free(self);
}

static void doMark(struct gc_per_generation_state* state, struct arena_block* block) {
  atomic_store(&block->gcMetadata.markBit, state->GCMarkedBitValue);
}

struct cycle_state {
  struct gc_per_generation_state* self;
  struct arena* arena;
  struct heap* heap;
  
  // Temporary stats stored here before finally copied to per generation state
  struct gc_stats stats;
  
  struct timespec pauseBegin, pauseEnd;
};

// Must be STW once GC has own thread
static void takeRootSnapshotPhase(struct cycle_state* state) {
  int ret;
  flup_mutex_lock(state->heap->rootLock);
  if ((ret = flup_dyn_array_reserve(state->self->snapshotOfRootSet, state->heap->rootEntryCount)) < 0)
    flup_panic("Error reserving memory for root set snapshot: flup_dyn_array_reserve: %d", ret);
  
  flup_list_head* current;
  flup_list_for_each(&state->heap->root, current) {
    struct root_ref* ref = container_of(current, struct root_ref, node);
    if (flup_dyn_array_append(state->self->snapshotOfRootSet, &ref) < 0)
      flup_panic("This can't occur TwT");
  }
  flup_mutex_unlock(state->heap->rootLock);
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
  uint64_t count = 0;
  size_t totalSize = 0;
  
  uint64_t sweepedCount = 0;
  size_t sweepSize = 0;
  
  uint64_t liveObjectCount = 0;
  size_t liveObjectSize = 0;
  
  for (size_t i = 0; i < state->arena->numBlocksCreated; i++) {
    struct arena_block* block = state->arena->blocks[i];
    // If block is invalid that mean it has to be recently allocated
    // so treat it as marked
    if (!block || !atomic_load(&block->gcMetadata.isValid))
      continue;
    
    count++;
    totalSize += block->size;
    // Object is alive continuing
    if (atomic_load(&block->gcMetadata.markBit) == state->self->GCMarkedBitValue) {
      liveObjectCount++;
      liveObjectSize += block->size;
      continue;
    }
    
    sweepedCount++;
    sweepSize += block->size;
    arena_dealloc(state->arena, block);
  }
  
  state->stats.lifetimeTotalSweepedObjectCount += sweepedCount;
  state->stats.lifetimeTotalSweepedObjectSize += sweepSize;
  
  state->stats.lifetimeTotalObjectCount += count;
  state->stats.lifetimeTotalObjectSize += totalSize;
  
  state->stats.lifetimeTotalLiveObjectCount += liveObjectCount;
  state->stats.lifetimeTotalObjectSize += liveObjectSize;
}

static void pauseAppThreads(struct cycle_state* state) {
  clock_gettime(CLOCK_REALTIME, &state->pauseBegin);
  flup_rwlock_wrlock(state->self->gcLock);
}

static void unpauseAppThreads(struct cycle_state* state) {
  flup_rwlock_unlock(state->self->gcLock);
  clock_gettime(CLOCK_REALTIME, &state->pauseEnd);
  
  double duration = 
    ((double) state->pauseEnd.tv_sec + ((double) state->pauseEnd.tv_nsec/ 1'000'000'000.0f)) -
    ((double) state->pauseBegin.tv_sec + ((double) state->pauseBegin.tv_nsec/ 1'000'000'000.0f));
  state->stats.lifetimeSTWTime += duration;
}

static void cycleRunner(struct gc_per_generation_state* self) {
  struct arena* arena = self->ownerGen->arena;
  struct heap* heap = self->ownerGen->ownerHeap;
  pthread_setname_np(pthread_self(), "GC-Thread");
  
  // pr_info("Before cycle mem usage: %f MiB", (float) atomic_load(&arena->currentUsage) / 1024.0f / 1024.0f);
  
  struct cycle_state state = {
    .arena = arena,
    .self = self,
    .heap = heap
  };
  
  flup_mutex_lock(self->statsLock);
  self->stats.cycleIsRunning = true;
  state.stats = self->stats;
  flup_mutex_unlock(self->statsLock);
  
  struct timespec start, end;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
  
  pauseAppThreads(&state);
  atomic_store(&self->mutatorMarkedBitValue, !atomic_load(&self->mutatorMarkedBitValue));
  atomic_store(&self->cycleInProgress, true);
  
  takeRootSnapshotPhase(&state);
  unpauseAppThreads(&state);
  
  markingPhase(&state);
  processMutatorMarkQueuePhase(&state);
  sweepPhase(&state);
  
  pauseAppThreads(&state);
  self->GCMarkedBitValue = !self->GCMarkedBitValue;
  atomic_store(&self->cycleInProgress, false);
  unpauseAppThreads(&state);
  
  flup_dyn_array_remove(self->snapshotOfRootSet, 0, self->snapshotOfRootSet->length);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
  
  double duration =
    ((double) end.tv_sec + ((double) end.tv_nsec) / 1'000'000'000.0f) -
    ((double) start.tv_sec + ((double) start.tv_nsec) / 1'000'000'000.0f);
  state.stats.lifetimeCycleTime += duration;
  
  flup_mutex_lock(self->statsLock);
  static struct timespec deadline = {};
  if (deadline.tv_sec == 0) {
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec -= 1;
  }
  
  struct timespec current;
  clock_gettime(CLOCK_REALTIME, &current);
  if (current.tv_sec >= deadline.tv_sec) {
    pr_info("STW (last second): %lf sec", state.stats.lifetimeSTWTime - self->stats.lifetimeSTWTime);
    pr_info("STW (lifetime)   : %lf sec", state.stats.lifetimeSTWTime);
    pr_info("GC rate          : %lf MiB/sec", (double) (state.stats.lifetimeTotalSweepedObjectSize - self->stats.lifetimeTotalSweepedObjectSize) / 1024.0f / 1024.0f);
    deadline.tv_sec++;
  }
  
  self->stats = state.stats;
  self->stats.cycleIsRunning = false;
  flup_mutex_unlock(self->statsLock);
  
  flup_mutex_lock(self->invokeCycleLock);
  self->cycleID++;
  self->cycleWasInvoked = false;
  flup_mutex_unlock(self->invokeCycleLock);
  flup_cond_wake_all(self->invokeCycleDoneEvent);
}

static void gcThread(void* _self) {
  struct gc_per_generation_state* self = _self;
  while (1) {
    flup_mutex_lock(self->gcRequestLock);
    while (self->gcRequest == GC_NOOP)
      flup_cond_wait(self->gcRequestedCond, self->gcRequestLock, NULL);
    enum gc_request request = self->gcRequest;
    self->gcRequest = GC_NOOP;
    flup_mutex_unlock(self->gcRequestLock);
    
    switch (request) {
      case GC_NOOP:
        flup_panic("Unreachable");
      case GC_SHUTDOWN:
        pr_info("Shutting down GC thread");
        goto shutdown_gc_thread;
      case GC_START_CYCLE:
        // pr_info("Starting GC cycle!");
        cycleRunner(self);
        break;
    }
  }
shutdown_gc_thread:
}

uint64_t gc_start_cycle_async(struct gc_per_generation_state* self) {
  // It was already started lets wait
  flup_mutex_lock(self->invokeCycleLock);
  uint64_t lastCycleID = self->cycleID;
  if (self->cycleWasInvoked) {
    flup_mutex_unlock(self->invokeCycleLock);
    goto no_need_to_wake_gc;
  }
  
  self->cycleWasInvoked = true;
  flup_mutex_unlock(self->invokeCycleLock);
  
  // Wake GC thread
  callGCAsync(self, GC_START_CYCLE);
no_need_to_wake_gc:
  return lastCycleID;
}

void gc_start_cycle(struct gc_per_generation_state* self) {
  uint64_t lastCycleID = gc_start_cycle_async(self);
  
  flup_mutex_lock(self->invokeCycleLock);
  // Waiting loop
  while (self->cycleID == lastCycleID)
    flup_cond_wait(self->invokeCycleDoneEvent, self->invokeCycleLock, NULL);
  flup_mutex_unlock(self->invokeCycleLock);
  return;
}

void gc_block(struct gc_per_generation_state* self) {
  flup_rwlock_rdlock(self->gcLock);
}

void gc_unblock(struct gc_per_generation_state* self) {
  flup_rwlock_unlock(self->gcLock);
}

void gc_get_stats(struct gc_per_generation_state* self, struct gc_stats* stats) {
  flup_mutex_lock(self->statsLock);
  *stats = self->stats;
  flup_mutex_unlock(self->statsLock);
}

