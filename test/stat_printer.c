#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include <flup/bug.h>
#include <flup/concurrency/cond.h>
#include <flup/concurrency/mutex.h>
#include <flup/core/logger.h>
#include <flup/core/panic.h>
#include <flup/thread/thread.h>

#include "stat_printer.h"
#include "heap/heap.h"

static void loopThread(void* _self);
struct stat_printer* stat_printer_new(struct heap* heap) {
  struct stat_printer* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  *self = (struct stat_printer) {
    .heap = heap,
  };
  
  if (!(self->requestLock = flup_mutex_new()))
    goto failure;
  if (!(self->statusLock = flup_mutex_new()))
    goto failure;
  if (!(self->requestNeededEvent = flup_cond_new()))
    goto failure;
  if (!(self->statusUpdatedEvent = flup_cond_new()))
    goto failure;
  if (!(self->printerThread = flup_thread_new(loopThread, self)))
    goto failure;
  return self;

failure:
  stat_printer_free(self);
  return NULL;
}

static enum stat_printer_status callRequest(struct stat_printer* self, enum stat_printer_request req) {
  flup_mutex_lock(self->requestLock);
  self->request = req;
  uint64_t prevReqID = self->reqCount;
  self->reqCount++;
  flup_mutex_unlock(self->requestLock);
  flup_cond_wake_one(self->requestNeededEvent);
  
  flup_mutex_lock(self->statusLock);
  // As long the latest executed request ID still the same as the moment
  // of the request it is sure nothing is executed yet
  while (prevReqID == self->latestExecutedReqID)
    flup_cond_wait(self->statusUpdatedEvent, self->statusLock, NULL);
  
  auto newStatus = self->status;
  flup_mutex_unlock(self->statusLock);
  return newStatus;
}

void stat_printer_free(struct stat_printer* self) {
  if (!self)
    return;
  
  auto res = callRequest(self, STAT_PRINTER_REQUEST_SHUTDOWN);
  BUG_ON(res != STAT_PRINTER_STATUS_SHUTDOWNED);
  
  flup_thread_wait(self->printerThread);
  flup_thread_free(self->printerThread);
  
  flup_mutex_free(self->requestLock);
  flup_mutex_free(self->statusLock);
  flup_cond_free(self->requestNeededEvent);
  flup_cond_free(self->statusUpdatedEvent);
  free(self);
}

void stat_printer_start(struct stat_printer* self) {
  callRequest(self, STAT_PRINTER_REQUEST_START);
}

void stat_printer_stop(struct stat_printer* self) {
  callRequest(self, STAT_PRINTER_REQUEST_STOP);
}

static void loopThread(void* _self) {
  struct stat_printer* self = _self;
  
  flup_mutex_lock(self->requestLock);
  
  // Control what the loop does if not processing requests
  bool shutdown = false;
  bool doTimedWait = false;
  
  // 100 milisec update interval
  const int intervalMilisec = 100;
  const time_t intervalSecs = intervalMilisec / 1'000;
  const long intervalNanosecs = (intervalMilisec % 1'000) * 1'000'000;
  
  struct timespec deadline;
  bool firstCommand = true;
  while (!shutdown) {
    int waitStatus = 0;
    uint64_t lastReqCount = firstCommand ? 0 : self->reqCount;
    while (waitStatus == 0 && lastReqCount == self->reqCount)
      waitStatus = flup_cond_wait(self->requestNeededEvent, self->requestLock, doTimedWait ? &deadline : NULL);
    firstCommand = false;
    
    if (waitStatus == -ETIMEDOUT)
      goto dont_process_request;
    
    enum stat_printer_status newStatus;
    switch (self->request) {
      case STAT_PRINTER_REQUEST_START:
        newStatus = STAT_PRINTER_STATUS_STARTED;
        doTimedWait = true;
        clock_gettime(CLOCK_REALTIME, &deadline);
        break;
      case STAT_PRINTER_REQUEST_STOP:
        newStatus = STAT_PRINTER_STATUS_STOPPED;
        doTimedWait = false;
        break;
      case STAT_PRINTER_REQUEST_SHUTDOWN:
        flup_mutex_unlock(self->statusLock);
        shutdown = true;
        newStatus = STAT_PRINTER_STATUS_SHUTDOWNED;
        break;
      default:
        flup_panic("Not implemented");
    }
    
    flup_mutex_lock(self->statusLock);
    self->status = newStatus;
    self->latestExecutedReqID++;
    flup_mutex_unlock(self->statusLock);
    flup_cond_wake_all(self->statusUpdatedEvent);
    
dont_process_request:
    if (!doTimedWait)
      continue;
    
    deadline.tv_nsec += intervalNanosecs;
    deadline.tv_sec  += intervalSecs + (deadline.tv_nsec / 1'000'000'000);
    deadline.tv_nsec %= 1'000'000'000;
    
    // Update display here
    pr_info("Hellooo");
  }
  flup_mutex_unlock(self->requestLock);
}

