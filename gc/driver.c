#include <stdatomic.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

#include <flup/core/panic.h>
#include <flup/core/logger.h>
#include <flup/thread/thread.h>

#include "driver.h"

#undef FLUP_LOG_CATEGORY
#define FLUP_LOG_CATEGORY "GC Driver"

// This file will contain logics to decide when to start GC
// while the gc.c primarily focus on the actual GC process

static void pollHeapState(struct gc_driver* self) {
  
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
    pollHeapState(self);
    
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
    .gcState = gcState
  };
  
  if (!(self->driverThread = flup_thread_new(driver, self)))
    goto failure;
  return self;
failure:
  gc_driver_free(self);
  return NULL;
}

void gc_driver_free(struct gc_driver* driver) {
  if (!driver)
    return;
  
  if (driver->driverThread) {
    atomic_store(&driver->quitRequested, true);
    
    // In this line, hope that driver don't overslept
    
    flup_thread_wait(driver->driverThread);
    flup_thread_free(driver->driverThread);
  }
  free(driver);
}

