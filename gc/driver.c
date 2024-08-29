#include <flup/util/min_max.h>
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
#include "gc/stat_collector.h"
#include "heap/generation.h"
#include "memory/alloc_tracker.h"
#include "util/moving_window.h"

#include "driver.h"

#undef FLUP_LOG_CATEGORY
#define FLUP_LOG_CATEGORY "GC Driver"

// This file will contain logics to decide when to start GC
// while the gc.c primarily focus on the actual GC process

static size_t calcAverageTargetThreshold(struct gc_driver* self) {
  if (self->triggerThresholdSamples->entryCount == 0)
    return 0;
  
  struct moving_window_iterator iterator = {};
  size_t total = 0;
  while (moving_window_next(self->triggerThresholdSamples, &iterator))
    total += *((size_t*) iterator.current);
  
  return total / self->triggerThresholdSamples->entryCount;
}

// Calculate second until "bytes" usage reached
// based on current allocation rates
static float calcTimeToUsage(struct gc_driver* self, size_t bytes) {
  float heapUsage = (float) atomic_load(&self->gcState->ownerGen->allocTracker->currentUsage);
  float allocRate = (float) atomic_load(&self->statCollector->averageAllocRatePerSecond) + 1;
  float bytesFloat = (float) bytes;
  
  return (bytesFloat - heapUsage) / allocRate;
}

// Get averaged cycle time with driver poll period as minimum
// because due some calculations and prediction only triggers
// based on predicted memory usage but if cycle is too fast than
// the poll period, driver may not be able to respond
static float getAdjustedAverageCycleTime(struct gc_driver* self) {
  float averageCycleTime = (float) atomic_load(&self->gcState->averageCycleTime);
  if (averageCycleTime < 1.0f / DRIVER_CHECK_RATE_HZ)
    averageCycleTime = 1.0f / DRIVER_CHECK_RATE_HZ;
  
  return averageCycleTime;
}

static thread_local struct timespec deadline;
static void doCollection(struct gc_driver* self) {
  atomic_store(&self->gcState->pacingMicrosec, 0);
  struct timespec gcBeginAtSpec;
  clock_gettime(CLOCK_REALTIME, &gcBeginAtSpec);
  float gcBeginTime = (float) gcBeginAtSpec.tv_sec + ((float) gcBeginAtSpec.tv_nsec / 1e9f);
  uint64_t cycleID = gc_start_cycle_async(self->gcState);
  
  float currentDelayMicrosec = 500;
  float multiplier = 1.3f;
  while (gc_wait_cycle(self->gcState, cycleID, &deadline) == -ETIMEDOUT) {
    struct timespec currentTimeSpec;
    clock_gettime(CLOCK_REALTIME, &currentTimeSpec);
    
    float timeSinceStart = ((float) currentTimeSpec.tv_sec + ((float) currentTimeSpec.tv_nsec / 1e9f)) - gcBeginTime;
    float averageCycleTime = getAdjustedAverageCycleTime(self);
    float timeUntilCycleCompletion = averageCycleTime - timeSinceStart;
    
    if (timeUntilCycleCompletion < 1.0f / DRIVER_CHECK_RATE_HZ)
      timeUntilCycleCompletion = 1.0f / DRIVER_CHECK_RATE_HZ;
    
    if (calcTimeToUsage(self, self->gcState->ownerGen->allocTracker->maxSize) < timeUntilCycleCompletion) {
      currentDelayMicrosec *= multiplier;
      
      // pr_info("Delayed by %f ms", currentDelayMicrosec / 1'000);
      // Cap pace by 10 milisec
      if (currentDelayMicrosec > 10'000)
        currentDelayMicrosec = 10'000;
      atomic_store_explicit(&self->gcState->pacingMicrosec, (unsigned int) currentDelayMicrosec, memory_order_relaxed);
    }
    
    deadline.tv_nsec += 1'000'000'000 / DRIVER_CHECK_RATE_HZ;
    if (deadline.tv_nsec >= 1'000'000'000) {
      deadline.tv_nsec -= 1'000'000'000;
      deadline.tv_sec++;
    }
  }
  // atomic_store(&self->gcState->pacingMicrosec, 0);
  
  size_t threshold = atomic_load(&self->gcState->bytesUsedRightBeforeSweeping);
  moving_window_append(self->triggerThresholdSamples, &threshold);
  
  atomic_store(&self->averagePeakMemoryBeforeCycle, calcAverageTargetThreshold(self));
  
  struct timespec currentTime;
  clock_gettime(CLOCK_REALTIME, &currentTime);
  self->lastCollectionTime = (double) currentTime.tv_sec + ((double) currentTime.tv_nsec / 1e9f);
  self->lastCycleHeapUsage = atomic_load(&self->gcState->liveSetSize);
}

static bool maxCollectIntervalRule(struct gc_driver* self) {
  struct timespec currentTimeSpec;
  clock_gettime(CLOCK_REALTIME, &currentTimeSpec);
  
  double currentTime = (double) currentTimeSpec.tv_sec + ((double) currentTimeSpec.tv_nsec / 1e9f);
  
  // Time since last collection is too long
  if (currentTime - self->lastCollectionTime > 5.0f) {
    doCollection(self);
    return true;
  }
  
  return false;
}

static bool lowMemoryRule(struct gc_driver* self) {
  struct generation* gen = self->gcState->ownerGen;
  
  size_t usage = atomic_load(&gen->allocTracker->currentUsage);
  size_t softLimit = (size_t) ((float) gen->allocTracker->maxSize * 0.95f);
  
  // Start GC cycle so memory freed before mutator has to start
  // waiting on GC 
  if (usage > softLimit) {
    pr_verbose("Low memory rule: starting GC");
    doCollection(self);
    return true;
  }
  return false;
}

// Runs GC at 10%, 20%, 30%, 40%, and 50% to warm up statistics
static bool warmUpRule(struct gc_driver* self) {
  static thread_local int warmUpCurrentCount = 0;
  if (warmUpCurrentCount >= 5)
    return false;
  
  struct generation* gen = self->gcState->ownerGen;
  
  float warmPercent = 0.10f + (float) warmUpCurrentCount * 0.10f;
  size_t usage = atomic_load(&gen->allocTracker->currentUsage);
  size_t warmTrigger = (size_t) ((float) gen->allocTracker->maxSize * warmPercent);
  
  if (usage > warmTrigger) {
    pr_verbose("Warming GC at %.00f percent", warmPercent * 100);
    doCollection(self);
    warmUpCurrentCount++;
    return true;
  }
  return false;
}

struct polling_state {
  double secondsToOOM;
  double adjustedCycleTime;
};

static void preRunMatchingRateRule(struct gc_driver* self, struct polling_state* state) {
  double rawBytesLimit = (double) atomic_load(&self->averagePeakMemoryBeforeCycle);
  double bytesLimit = rawBytesLimit * 0.5f;
  
  double cycleTime = atomic_load(&self->gcState->averageCycleTime);
  double allocRate = (double) (atomic_load(&self->statCollector->averageAllocRatePerSecond) + 1);
  if (bytesLimit > (double) self->gcState->ownerGen->allocTracker->maxSize)
    bytesLimit = (double) self->gcState->ownerGen->allocTracker->maxSize;
  
  double bytesToOOM = bytesLimit - (double) atomic_load(&self->gcState->ownerGen->allocTracker->currentUsage);
  if (bytesToOOM < 0)
    bytesToOOM = 0;
  double secondsToOOM = (double) bytesToOOM / allocRate;
  
  state->adjustedCycleTime = cycleTime;
  state->secondsToOOM = secondsToOOM;
}

static bool matchingRateRule(struct gc_driver* self, struct polling_state* state) {
  if (state->secondsToOOM < 1.0f / DRIVER_CHECK_RATE_HZ) {
    doCollection(self);
    return true;
  }
  
  if (state->secondsToOOM < state->adjustedCycleTime) {
    doCollection(self);
    return true;
  }
  return false;
}

static bool growthRule(struct gc_driver* self) {
  float heapSize = (float) self->gcState->ownerGen->allocTracker->maxSize;
  float heapUsage = (float) atomic_load(&self->gcState->ownerGen->allocTracker->currentUsage);
  float allocRate = (float) atomic_load(&self->statCollector->averageAllocRatePerSecond) + 1;
  
  float nextTime = (float) atomic_load(&self->gcState->averageCycleTime);
  if (nextTime < 1.0f / DRIVER_CHECK_RATE_HZ)
    nextTime = 1.0f / DRIVER_CHECK_RATE_HZ;
  float heapUsageByNextTime = heapUsage + allocRate * nextTime;
  float heapUsageSinceLastGrowth = (float) self->lastCycleHeapUsageSinceLastGrowthTrigger;
  
  // Grown by 30% of free space
  // or 40% of heap growth whichever is smallest
  float minGrowth = (heapSize - heapUsage) * 0.30f;
  if (heapUsageSinceLastGrowth * 0.40f < minGrowth)
    minGrowth = heapUsageSinceLastGrowth * 0.40f;
  
  if (heapUsageByNextTime > heapUsageSinceLastGrowth + minGrowth) {
    doCollection(self);
    self->lastCycleHeapUsageSinceLastGrowthTrigger = self->lastCycleHeapUsage;
    return true;
  }
  
  return false;
}

static void pollHeapState(struct gc_driver* self) {
  struct polling_state state = {};
  preRunMatchingRateRule(self, &state);
  
  // Trigger on low memory in case other rule is too slow to respond
  if (lowMemoryRule(self))
    return;
  
  // Triggers N cycles unconditonally from start and never
  // to collect cycle times and other statistics
  if (warmUpRule(self))
    return;
  
  // Match GC with alloc rate
  // if (matchingRateRule(self, &state))
  //  return;
  
  // Trigger GC after % free space is used for case
  // when match rate is too slow
  if (growthRule(self))
    return;
  
  // Trigger GC unconditionally after certain interval
  if (maxCollectIntervalRule(self))
    return;
}

static void driver(void* _self) {
  struct gc_driver* self = _self;
  
  if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
    flup_panic("Strange this implementation did not support CLOCK_REALTIME");
  
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
    while ((ret = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &deadline, NULL)) == EINTR)
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
  
  if (!(self->triggerThresholdSamples = moving_window_new(sizeof(size_t), DRIVER_TRIGGER_THRESHOLD_SAMPLES)))
    goto failure;
  
  if (!(self->statCollector = stat_collector_new(gcState)))
    goto failure;
  if (!(self->driverThread = flup_thread_new(driver, self)))
    goto failure;
  return self;
failure:
  gc_driver_free(self);
  return NULL;
}

void gc_driver_unpause(struct gc_driver* self) {
  stat_collector_unpause(self->statCollector);
  atomic_store(&self->paused, false);
}

void gc_driver_perform_shutdown(struct gc_driver* self) {
  gc_driver_unpause(self);
  atomic_store(&self->quitRequested, true);
  
  if (self->driverThread)
    flup_thread_wait(self->driverThread);
  
  stat_collector_perform_shutdown(self->statCollector);
}

void gc_driver_free(struct gc_driver* self) {
  if (!self)
    return;
  
  if (self->driverThread)
    flup_thread_free(self->driverThread);
  stat_collector_free(self->statCollector);
  moving_window_free(self->triggerThresholdSamples);
  free(self);
}

