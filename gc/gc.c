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
#include <flup/data_structs/buffer/circular_buffer.h>
#include <flup/data_structs/dyn_array.h>
#include <flup/data_structs/buffer.h>

#include "heap/heap.h"
#include "heap/thread.h"
#include "memory/arena.h"
#include "heap/generation.h"
#include "object/descriptor.h"

#include "gc.h"

void gc_on_allocate(struct alloc_unit* block, struct generation* gen) {
  block->gcMetadata.markBit = !gen->gcState->mutatorMarkedBitValue;
  block->gcMetadata.owningGeneration = gen;
  
  // Don't start async cycle if one in progress
  if (atomic_load(&gen->gcState->cycleInProgress))
    return;
  
  size_t usage = atomic_load(&gen->arena->currentUsage);
  size_t maxSize = gen->arena->maxSize;
  size_t softLimit = (size_t) ((float) maxSize * gen->gcState->asyncTriggerThreshold);
  
  // Start GC cycle so memory freed before mutator has to start
  // waiting on GC 
  if (usage > softLimit) {
    gc_start_cycle_async(gen->gcState);
  }
}

void gc_need_remark(struct alloc_unit* obj) {
  if (!obj)
    return;
  
  struct gc_block_metadata* metadata = &obj->gcMetadata;
  struct gc_per_generation_state* gcState = metadata->owningGeneration->gcState;
  
  // Add to queue if marking in progress
  if (!atomic_load(&gcState->markingInProgress))
    return;
  
  bool prevMarkBit = atomic_exchange(&metadata->markBit, gcState->GCMarkedBitValue);
  if (prevMarkBit == gcState->GCMarkedBitValue)
    return;
  
  // Enqueue an pointer
  flup_buffer_write_no_fail(gcState->needRemarkQueue, &obj, sizeof(void*));
}

static void gcThread(void* _self);
struct gc_per_generation_state* gc_per_generation_state_new(struct generation* gen) {
  struct gc_per_generation_state* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_per_generation_state) {
    .ownerGen = gen,
    .asyncTriggerThreshold = 0.7f
  };
  
  if (!(self->gcLock = flup_rwlock_new()))
    goto failure;
  if (!(self->needRemarkQueue = flup_buffer_new(GC_MUTATOR_MARK_QUEUE_SIZE)))
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
  if (!(self->gcMarkQueueUwU = flup_circular_buffer_new(GC_MARK_QUEUE_SIZE)))
    goto failure;
  if (!(self->deferredMarkQueue = flup_circular_buffer_new(GC_DEFERRED_MARK_QUEUE_SIZE)))
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
  flup_cond_wake_one(self->gcRequestedCond);
  flup_mutex_unlock(self->gcRequestLock);
}

void gc_per_generation_state_free(struct gc_per_generation_state* self) {
  if (!self)
    return;
  
  if (self->thread) {
    callGCAsync(self, GC_SHUTDOWN);
    flup_thread_wait(self->thread);
    flup_thread_free(self->thread);
  }
  flup_circular_buffer_free(self->gcMarkQueueUwU);
  flup_circular_buffer_free(self->deferredMarkQueue);
  flup_mutex_free(self->statsLock);
  flup_cond_free(self->gcRequestedCond);
  flup_mutex_free(self->gcRequestLock);
  flup_cond_free(self->invokeCycleDoneEvent);
  flup_mutex_free(self->invokeCycleLock);
  flup_rwlock_free(self->gcLock);
  free(self->snapshotOfRootSet);
  flup_buffer_free(self->needRemarkQueue);
  free(self);
}

static bool markOneItem(struct gc_per_generation_state* state, struct alloc_unit* parent, size_t parentIndex, struct alloc_unit* fieldContent) {
  if (!fieldContent)
    return true;
  
  int ret;
  if ((ret = flup_circular_buffer_write(state->gcMarkQueueUwU, &fieldContent, sizeof(void*))) < 0) {
    struct gc_mark_state savedState = {
      .block = parent,
      .fieldIndex = parentIndex
    };
    
    // If mark queue can't fit just put it in deferred mark queue
    if ((ret = flup_circular_buffer_write(state->deferredMarkQueue, &savedState, sizeof(savedState))) < 0) {
      size_t numOfFieldsToTriggerMarkQueueOverflow = (GC_MARK_QUEUE_SIZE / sizeof(void*)) + 1;
      size_t numOfObjectsToTriggerRemarkQueueOverflow = (GC_DEFERRED_MARK_QUEUE_SIZE / sizeof(struct gc_mark_state)) + 1;
      size_t perObjectBytesToTriggerMarkQueueOverflow = numOfFieldsToTriggerMarkQueueOverflow * sizeof(void*);
      size_t totalBytesToTriggerRemarkQueueOverflow = numOfObjectsToTriggerRemarkQueueOverflow * perObjectBytesToTriggerMarkQueueOverflow;
      flup_panic("!!Congrat!! You found very absurb condition with ~%lf TiB worth of bytes composed from %zu objects sized %zu bytes each (or %zu fields/array entries each) UwU", ((double) totalBytesToTriggerRemarkQueueOverflow) / 1024.0f / 1024.0f / 1024.0f / 1024.0f, numOfObjectsToTriggerRemarkQueueOverflow, perObjectBytesToTriggerMarkQueueOverflow, numOfFieldsToTriggerMarkQueueOverflow);
    }
    
    return false;
  }
  return true;
}

static void doMarkInner(struct gc_per_generation_state* state, struct gc_mark_state* markState) {
  struct alloc_unit* block = markState->block;
  bool markBit = atomic_exchange(&block->gcMetadata.markBit, state->GCMarkedBitValue);
  if (markState->fieldIndex == 0 && markBit == state->GCMarkedBitValue)
    return;
  
  struct descriptor* desc = atomic_load(&block->desc);
  // Object have no GC-able references
  if (!desc)
    return;
  
  // Uses breadth first search but if failed
  // queue current state to process later
  size_t fieldIndex = 0;
  for (fieldIndex = markState->fieldIndex; fieldIndex < desc->fieldCount; fieldIndex++) {
    size_t offset = desc->fields[fieldIndex].offset;
    _Atomic(struct alloc_unit*)* fieldPtr = (_Atomic(struct alloc_unit*)*) ((void*) (((char*) block->data) + offset));
    if (!markOneItem(state, block, fieldIndex, atomic_load(fieldPtr)))
      return;
  }
  
  if (!desc->hasFlexArrayField)
    return;
  
  size_t flexArrayCount = (block->size - desc->objectSize) / sizeof(void*);
  for (size_t i = 0; i < flexArrayCount; i++) {
    _Atomic(struct alloc_unit*)* fieldPtr = (_Atomic(struct alloc_unit*)*) ((void*) (((char*) block->data) + desc->objectSize + i * sizeof(void*)));
    if (!markOneItem(state, block, fieldIndex + i, atomic_load(fieldPtr)))
      return;
  }
}

static void processMarkQueue(struct gc_per_generation_state* state) {
  int ret;
  struct alloc_unit* current;
  while ((ret = flup_circular_buffer_read(state->gcMarkQueueUwU, &current, sizeof(void*))) == 0) {
    struct gc_mark_state markState = {
      .block = current,
      .fieldIndex = 0
    };
    doMarkInner(state, &markState);
  }
  
  if (ret != -ENODATA)
    flup_panic("Error reading GC mark queue: %d", ret);
}

static void doMark(struct gc_per_generation_state* state, struct alloc_unit* block) {
  int ret;
  if ((ret = flup_circular_buffer_write(state->gcMarkQueueUwU, &block, sizeof(void*))) < 0)
    flup_panic("Cannot enqueue to GC mark queue (configured queue size was %zu bytes): %d", (size_t) GC_MARK_QUEUE_SIZE, ret);
  
  processMarkQueue(state);
  
  struct gc_mark_state current;
  while ((ret = flup_circular_buffer_read(state->deferredMarkQueue, &current, sizeof(current))) == 0) {
    doMarkInner(state, &current);
    processMarkQueue(state);
  }
  
  if (ret != -ENODATA)
    flup_panic("Error reading GC deferred mark queue: %d", ret);
}

struct cycle_state {
  struct gc_per_generation_state* self;
  struct alloc_tracker* arena;
  struct heap* heap;
  
  // Temporary stats stored here before finally copied to per generation state
  struct gc_stats stats;
  
  struct timespec pauseBegin, pauseEnd;
  
  struct alloc_unit* detachedHeadToBeSwept;
};

static void takeRootSnapshotPhase(struct cycle_state* state) {
  struct alloc_unit** rootSnapshot = realloc(state->self->snapshotOfRootSet, state->heap->mainThread->rootSize * sizeof(void*));
  if (!rootSnapshot)
    flup_panic("Error reserving memory for root set snapshot");
  state->self->snapshotOfRootSet = rootSnapshot;
  state->self->snapshotOfRootSetSize = state->heap->mainThread->rootSize;
  
  size_t index = 0;
  flup_list_head* current;
  flup_list_for_each(&state->heap->mainThread->rootEntries, current) {
    rootSnapshot[index] = container_of(current, struct root_ref, node)->obj;
    index++;
  }
}

static void markingPhase(struct cycle_state* state) {
  for (size_t i = 0; i < state->self->snapshotOfRootSetSize; i++)
    doMark(state->self, state->self->snapshotOfRootSet[i]);
}

static void processMutatorMarkQueuePhase(struct cycle_state* state) {
  struct alloc_unit* current;
  int ret;
  while ((ret = flup_buffer_read2(state->self->needRemarkQueue, &current, sizeof(void*), FLUP_BUFFER_READ2_DONT_WAIT_FOR_DATA)) >= 0) {
    atomic_store(&current->gcMetadata.markBit, !state->self->GCMarkedBitValue);
    doMark(state->self, current);
  }
  BUG_ON(ret == -EMSGSIZE);
}

static void sweepPhase(struct cycle_state* state) {
  uint64_t count = 0;
  size_t totalSize = 0;
  
  uint64_t sweepedCount = 0;
  size_t sweepSize = 0;
  
  uint64_t liveObjectCount = 0;
  size_t liveObjectSize = 0;
  
  struct alloc_unit* block = state->detachedHeadToBeSwept;
  struct alloc_unit* next;
  while (block) {
    next = block->next;
    
    count++;
    totalSize += block->size;
    // Mark must be invalid because when head is detached app is blocked from
    // creating new objects
    // Object is alive continuing
    if (atomic_load(&block->gcMetadata.markBit) == state->self->GCMarkedBitValue) {
      liveObjectCount++;
      liveObjectSize += block->size;
      
      // Add the same block back to real head
      arena_move_one_block_from_detached_to_real_head(state->arena, block);
      goto continue_to_next;
    }
    
    sweepedCount++;
    sweepSize += block->size;
    
    arena_dealloc(state->arena, block);
    
continue_to_next:
    block = next;
  }
  
  state->stats.lifetimeTotalSweepedObjectCount += sweepedCount;
  state->stats.lifetimeTotalSweepedObjectSize += sweepSize;
  
  state->stats.lifetimeTotalObjectCount += count;
  state->stats.lifetimeTotalObjectSize += totalSize;
  
  state->stats.lifetimeTotalLiveObjectCount += liveObjectCount;
  state->stats.lifetimeTotalObjectSize += liveObjectSize;
}

static void pauseAppThreads(struct cycle_state* state) {
  flup_rwlock_wrlock(state->self->gcLock);
  clock_gettime(CLOCK_REALTIME, &state->pauseBegin);
}

static void unpauseAppThreads(struct cycle_state* state) {
  clock_gettime(CLOCK_REALTIME, &state->pauseEnd);
  flup_rwlock_unlock(state->self->gcLock);
  
  double duration = 
    ((double) state->pauseEnd.tv_sec + ((double) state->pauseEnd.tv_nsec/ 1'000'000'000.0f)) -
    ((double) state->pauseBegin.tv_sec + ((double) state->pauseBegin.tv_nsec/ 1'000'000'000.0f));
  state->stats.lifetimeSTWTime += duration;
}

static void cycleRunner(struct gc_per_generation_state* self) {
  struct cycle_state state = {
    .arena = self->ownerGen->arena,
    .self = self,
    .heap = self->ownerGen->ownerHeap
  };
  
  // pr_info("Before cycle mem usage: %f MiB", (float) atomic_load(&state.arena->currentUsage) / 1024.0f / 1024.0f);
  
  flup_mutex_lock(self->statsLock);
  self->stats.cycleIsRunning = true;
  state.stats = self->stats;
  flup_mutex_unlock(self->statsLock);
  
  struct timespec start, end;
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
  
  pauseAppThreads(&state);
  self->mutatorMarkedBitValue = self->GCMarkedBitValue;
  atomic_store(&self->cycleInProgress, true);
  
  atomic_store(&self->markingInProgress, true);
  takeRootSnapshotPhase(&state);
  struct alloc_tracker_detached_head detached;
  arena_detach_head(state.arena, &detached);
  state.detachedHeadToBeSwept = detached.head;
  unpauseAppThreads(&state);
  
  markingPhase(&state);
  atomic_store(&self->markingInProgress, false);
  processMutatorMarkQueuePhase(&state);
  sweepPhase(&state);
  
  pauseAppThreads(&state);
  self->GCMarkedBitValue = !self->GCMarkedBitValue;
  atomic_store(&self->cycleInProgress, false);
  unpauseAppThreads(&state);
  clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
  
  double duration =
    ((double) end.tv_sec + ((double) end.tv_nsec) / 1'000'000'000.0f) -
    ((double) start.tv_sec + ((double) start.tv_nsec) / 1'000'000'000.0f);
  state.stats.lifetimeCycleTime += duration;
  
  flup_mutex_lock(self->statsLock);
  static struct timespec deadline = {};
  static struct gc_stats prevStats;
  if (deadline.tv_sec == 0) {
    clock_gettime(CLOCK_REALTIME, &deadline);
    deadline.tv_sec -= 1;
    prevStats = state.stats;
  }
  
  struct timespec current;
  clock_gettime(CLOCK_REALTIME, &current);
  if (current.tv_sec >= deadline.tv_sec) {
    pr_info("STW (last second): %lf sec", state.stats.lifetimeSTWTime - prevStats.lifetimeSTWTime);
    pr_info("STW (lifetime)   : %lf sec", state.stats.lifetimeSTWTime);
    pr_info("GC rate          : %lf MiB/sec", (double) (state.stats.lifetimeTotalSweepedObjectSize - prevStats.lifetimeTotalSweepedObjectSize) / 1024.0f / 1024.0f);
    deadline.tv_sec++;
    prevStats = state.stats;
  }
  
  self->stats = state.stats;
  self->stats.cycleIsRunning = false;
  flup_mutex_unlock(self->statsLock);
  
  flup_mutex_lock(self->invokeCycleLock);
  self->cycleID++;
  self->cycleWasInvoked = false;
  flup_cond_wake_all(self->invokeCycleDoneEvent);
  flup_mutex_unlock(self->invokeCycleLock);
  
  // pr_info("After cycle mem usage: %f MiB", (float) atomic_load(&state.arena->currentUsage) / 1024.0f / 1024.0f);
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

