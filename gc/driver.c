#include <limits.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>
#include <stddef.h>

#include <flup/core/panic.h>
#include <flup/core/logger.h>
#include <flup/thread/thread.h>

#include "gc/gc.h"
#include "heap/generation.h"
#include "memory/alloc_tracker.h"
#include "util/moving_window.h"

#include "driver.h"

#undef FLUP_LOG_CATEGORY
#define FLUP_LOG_CATEGORY "GC Driver"

// This file will contain logics to decide when to start GC
// while the gc.c primarily focus on the actual GC process

// The unit returned is bytes per 1/DRIVER_CHECK_RATE_HZ
static size_t calcAllocationRatePerDriverHZ(struct gc_driver* self) {
  // Assume worst allocation rate because Foxie have no sample here
  struct moving_window_iterator iterator = {};
  size_t totalRates = 0;
  while (moving_window_next(self->runningSamplesOfAllocRates, &iterator))
    totalRates += *((size_t*) iterator.current);
  
  return totalRates / self->runningSamplesOfAllocRates->entryCount;
}

static unsigned int calcAverageCycleTime(struct gc_driver* self) {
  // Assume GC cycle take fastest time because Foxie have no sample here
  if (self->runningSamplesOfGCCycleTime->entryCount == 0)
    return 1;
  
  struct moving_window_iterator iterator = {};
  unsigned int totalCycleTime = 0;
  while (moving_window_next(self->runningSamplesOfGCCycleTime, &iterator))
    totalCycleTime += *((unsigned int*) iterator.current);
  
  return totalCycleTime / self->runningSamplesOfGCCycleTime->entryCount;
}

static unsigned int calcMicrosecBeforeOOM(size_t allocRate, size_t currentUsage, size_t totalAvailable) {
  size_t allocRateInMicrosec = allocRate * DRIVER_CHECK_RATE_HZ / 1'000'000;
  size_t microsecs = (totalAvailable - currentUsage) / allocRateInMicrosec;
  if (microsecs >= UINT_MAX)
    microsecs = UINT_MAX;
  return (unsigned int) microsecs;
}

static void doCollection(struct gc_driver* self) {
  struct gc_stats stats;
  double startTime, endTime;
  uint64_t startCycleCount, endCycleCount;
  
  gc_get_stats(self->gcState, &stats);
  startTime = stats.lifetimeCycleTime;
  startCycleCount = stats.lifetimeCyclesCount;
  
  gc_start_cycle(self->gcState);
  
  gc_get_stats(self->gcState, &stats);
  endTime = stats.lifetimeCycleTime;
  endCycleCount = stats.lifetimeCyclesCount;
  
  // The division ensure that if we miss some number of cycles
  // because of e.g. mutator trigger GC inseatd driver
  //
  // The cons of this is not all cycle times accurately sampled
  double cycleTimeInMicrosecsDouble = (endTime - startTime) / (double) (endCycleCount - startCycleCount) * 1'000'000;
  unsigned int cycleTimeInMicroseconds;
  if (cycleTimeInMicrosecsDouble < 1)
    cycleTimeInMicroseconds = 1; // Cycle time which takes 0 microseconds just impossible, round up
  else if (cycleTimeInMicrosecsDouble > UINT_MAX)
    cycleTimeInMicroseconds = UINT_MAX;
  else
    cycleTimeInMicroseconds = (unsigned int) cycleTimeInMicrosecsDouble;
  
  moving_window_append(self->runningSamplesOfGCCycleTime, &cycleTimeInMicroseconds);
}

static void pollHeapState(struct gc_driver* self) {
  struct generation* gen = self->gcState->ownerGen;
  // Capture allocation rates
  size_t current = atomic_load(&gen->allocTracker->lifetimeBytesAllocated);
  size_t rate = current - self->prevAllocBytes + 1;
  self->prevAllocBytes = current;
  moving_window_append(self->runningSamplesOfAllocRates, &rate);
  
  
  size_t usage = atomic_load(&gen->allocTracker->currentUsage);
  size_t softLimit = (size_t) ((float) gen->allocTracker->maxSize * gen->gcState->asyncTriggerThreshold);
  
  // Start GC cycle so memory freed before mutator has to start
  // waiting on GC 
  if (usage > softLimit) {
    size_t allocRate = calcAllocationRatePerDriverHZ(self);
    size_t currentUsage = atomic_load(&gen->allocTracker->currentUsage);
    size_t maxSize = gen->allocTracker->maxSize;
    unsigned int microsecBeforeOOM = calcMicrosecBeforeOOM(allocRate, currentUsage, maxSize);
    unsigned int microsecPredictedCycleTime = calcAverageCycleTime(self);
    pr_verbose("Alloc rate %.02f", (float) allocRate * DRIVER_CHECK_RATE_HZ / 1024 / 1024);
    pr_verbose("Soft limit reached, starting GC (System was %f seconds away from OOM and GC would take %f seconds)", (float) microsecBeforeOOM / 1'000'000, (float) microsecPredictedCycleTime / 1'000'000);
    doCollection(self);
    return;
  }
}

static void driver(void* _self) {
  struct gc_driver* self = _self;
  
  struct timespec deadline;
  clockid_t clockToUse;
  
  pr_info("Checking for CLOCK_MONOTONIC support");
  if (clock_gettime(CLOCK_MONOTONIC, &deadline) == 0) {
    pr_info("CLOCK_MONOTONIC available using it");
    clockToUse = CLOCK_MONOTONIC;
    goto nice_clock_found;
  }
  pr_info("CLOCK_MONOTONIC support unavailable falling back to CLOCK_REALTIME");
  
  // Fall back to CLOCK_REALTIME which should be available on most implementations
  if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
    flup_panic("Strange this implementation did not support CLOCK_REALTIME");
  
  clockToUse = CLOCK_REALTIME;
nice_clock_found:
  while (atomic_load(&self->quitRequested) == false) {
    if (atomic_load(&self->paused) == true)
      goto driver_was_paused;
    
    pollHeapState(self);
    
driver_was_paused:
    // Any neater way to deal this??? TwT
    deadline.tv_nsec += 1'000'000'000 / DRIVER_CHECK_RATE_HZ;
    if (deadline.tv_nsec >= 1'000'000'000) {
      deadline.tv_nsec -= 1'000'000'000;
      deadline.tv_sec++;
    }
    
    int ret = 0;
    while ((ret = clock_nanosleep(clockToUse, TIMER_ABSTIME, &deadline, NULL)) == EINTR)
      ;
    
    if (ret != 0)
      flup_panic("clock_nanosleep failed: %d", ret);
  }
  
  pr_info("Requested to quit, quiting!");
}

struct gc_driver* gc_driver_new(struct gc_per_generation_state* gcState) {
  struct gc_driver* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct gc_driver) {
    .gcState = gcState,
    .paused = true,
  };
  
  if (!(self->runningSamplesOfAllocRates = moving_window_new(sizeof(size_t), DRIVER_ALLOC_RATE_SAMPLES)))
    goto failure;
  if (!(self->runningSamplesOfGCCycleTime = moving_window_new(sizeof(unsigned int), DRIVER_CYCLE_TIME_SAMPLES)))
    goto failure;
  if (!(self->driverThread = flup_thread_new(driver, self)))
    goto failure;
  return self;
failure:
  gc_driver_free(self);
  return NULL;
}

void gc_driver_unpause(struct gc_driver* self) {
  atomic_store(&self->paused, false);
}

void gc_driver_perform_shutdown(struct gc_driver* self) {
  gc_driver_unpause(self);
  atomic_store(&self->quitRequested, true);
  
  if (self->driverThread)
    flup_thread_wait(self->driverThread);
}

void gc_driver_free(struct gc_driver* self) {
  if (!self)
    return;
  
  if (self->driverThread)
    flup_thread_free(self->driverThread);
  moving_window_free(self->runningSamplesOfAllocRates);
  moving_window_free(self->runningSamplesOfGCCycleTime);
  free(self);
}

