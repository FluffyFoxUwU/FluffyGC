#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <stdatomic.h>

#include <flup/thread/thread.h>
#include <flup/core/logger.h>
#include <flup/core/panic.h>

#include "memory/alloc_tracker.h"
#include "heap/generation.h"
#include "gc/gc.h"
#include "util/moving_window.h"
#include "stat_collector.h"

#undef FLUP_LOG_CATEGORY
#define FLUP_LOG_CATEGORY "GC Driver"

static void collectData(struct stat_collector* self) {
  static thread_local size_t lastLifetimeBytes = 0;
  size_t current = atomic_load(&self->gcState->ownerGen->allocTracker->lifetimeBytesAllocated);
  size_t rate = current - lastLifetimeBytes;
  lastLifetimeBytes = current;
  
  moving_window_append(self->allocRateSamples, &rate);
  
  struct moving_window_iterator iterator = {};
  size_t total = 0;
  while (moving_window_next(self->allocRateSamples, &iterator))
    total += *((size_t*) iterator.current);
  
  size_t averagedRateConverted = total * STAT_COLLECTOR_HZ / self->allocRateSamples->entryCount;
  atomic_store(&self->averageAllocRatePerSecond, averagedRateConverted);
  
  pr_info("Alloc rate is: %.02f MiB/s", (float) averagedRateConverted / 1024 / 1024);
}

static void statCollectorThread(void* _self) {
  struct stat_collector* self = _self;
  
  struct timespec deadline;
  if (clock_gettime(CLOCK_REALTIME, &deadline) != 0)
    flup_panic("Strange this implementation did not support CLOCK_REALTIME");
  
  while (!atomic_load(&self->quitRequested)) {
    if (atomic_load(&self->paused))
      goto stat_collector_was_paused;
    
    collectData(self);
    
stat_collector_was_paused:
    // Any neater way to deal this??? TwT
    deadline.tv_nsec += 1'000'000'000 / STAT_COLLECTOR_HZ;
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
  
  pr_info("Quit requested, quiting");
}

struct stat_collector* stat_collector_new(struct gc_per_generation_state* gcState) {
  struct stat_collector* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  
  *self = (struct stat_collector) {
    .gcState = gcState,
    .paused = true
  };
  
  if (!(self->allocRateSamples = moving_window_new(sizeof(size_t), STAT_COLLECTOR_ALLOC_RATE_SAMPLES)))
    goto failure;
  if (!(self->thread = flup_thread_new(statCollectorThread, self)))
    goto failure;
  return self;

failure:
  stat_collector_free(self);
  return NULL;
}

void stat_collector_unpause(struct stat_collector* self) {
  atomic_store(&self->paused, false);
}

void stat_collector_perform_shutdown(struct stat_collector* self) {
  stat_collector_unpause(self);
  if (self->thread)
    flup_thread_wait(self->thread);
}

void stat_collector_free(struct stat_collector* self) {
  if (!self)
    return;
  
  flup_thread_free(self->thread);
  moving_window_free(self->allocRateSamples);
  free(self);
}

