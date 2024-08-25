#include <errno.h>
#include <flup/util/min_max.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stddef.h>

#include <flup/bug.h>
#include <flup/data_structs/buffer/circular_buffer.h>
#include <flup/concurrency/cond.h>
#include <flup/concurrency/mutex.h>
#include <flup/core/logger.h>
#include <flup/core/panic.h>
#include <flup/thread/thread.h>

#include <SDL2/SDL_render.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_timer.h>
#include <SDL2/SDL_surface.h>

#include "gc/driver.h"
#include "stat_printer.h"
#include "gc/gc.h"
#include "heap/heap.h"
#include "memory/alloc_tracker.h"
#include "util/moving_window.h"

#define WIDTH 800
#define HEIGHT 600

static void loopThread(void* _self);
struct stat_printer* stat_printer_new(struct heap* heap) {
  struct stat_printer* self = malloc(sizeof(*self));
  if (!self)
    return NULL;
  *self = (struct stat_printer) {
    .heap = heap,
    .reqCount = 1,
    .request = STAT_PRINTER_REQUEST_START
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
  
  auto window = SDL_CreateWindow("Heap Usage", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_RenderSetVSync(renderer, 0);
  
  // Create moving window of WIDTH amount of data points
  struct moving_window* heapUsagePointsWindow = moving_window_new(sizeof(float), WIDTH);
  struct moving_window* gcThresholdPointsWindow  = moving_window_new(sizeof(float), WIDTH);
  struct moving_window* averageCycleTimeWindow = moving_window_new(sizeof(float), WIDTH);
  struct moving_window* proactiveGCThresholdPointsWindow = moving_window_new(sizeof(float), WIDTH);
  struct moving_window* liveSetSizeWindow = moving_window_new(sizeof(float), WIDTH);
  
  size_t heapSize = self->heap->gen->allocTracker->maxSize;
  struct alloc_tracker* tracker = self->heap->gen->allocTracker;
  struct gc_per_generation_state* gcState = self->heap->gen->gcState;
  
  auto recordData = ^() {
    struct alloc_tracker_statistic heapStats = {};
    alloc_tracker_get_statistics(tracker, &heapStats);
    size_t gcThreshold = atomic_load(&gcState->driver->averageTriggerThreshold);
    float averageCycleTime = (float) atomic_load(&gcState->averageCycleTime);
    
    struct gc_stats gcStats = {};
    gc_get_stats(gcState, &gcStats);
    
    float usagePercentage = ((float) heapStats.usedBytes) / ((float) heapSize);
    float gcThresholdPercentage = ((float) gcThreshold) / ((float) heapSize);
    float proactiveGCPercentage = ((float) atomic_load(&gcState->driver->averageProactiveGCThreshold)) / ((float) heapSize);
    float liveSetSize = ((float) atomic_load(&gcState->liveSetSize)) / ((float) heapSize);
    
    moving_window_append(heapUsagePointsWindow, &usagePercentage);
    moving_window_append(gcThresholdPointsWindow, &gcThresholdPercentage);
    moving_window_append(averageCycleTimeWindow, &averageCycleTime);
    moving_window_append(proactiveGCThresholdPointsWindow, &proactiveGCPercentage);
    moving_window_append(liveSetSizeWindow, &liveSetSize);
  };
  
  enum graph_kind {
    GRAPH_LINE,
    GRAPH_FILL
  };
  
  auto drawGraph = ^(enum graph_kind kind, struct moving_window* dataWindow) {
    struct moving_window_iterator iterator = {};
    int currentX = 0;
    float prevY;
    
    while (moving_window_next(dataWindow, &iterator)) {
      int prevX;
      float currentY = *(float*) iterator.current;
      if (currentX == 0) {
        prevY = currentY;
        prevX = 0;
      } else {
        prevX = currentX - 1;
      }
      
      switch (kind) {
        case GRAPH_FILL:
        case GRAPH_LINE:
          SDL_RenderDrawLine(renderer, 
            prevX, flup_max_int((int) ((1.0f - prevY) * HEIGHT), 0),
            currentX, flup_max_int((int) ((1.0f - currentY) * HEIGHT), 0)
          );
          break;
          SDL_RenderDrawLine(renderer, 
            currentX, 0,
            currentX, flup_max_int((int) ((1.0f - currentY) * HEIGHT), 0)
          );
          break;
      }
      
      currentX++;
      prevY = currentY;
    }
  };
  
  while (!shutdown) {
    int waitStatus = 0;
    uint64_t lastReqCount = firstCommand ? 0 : self->reqCount;
    while (waitStatus == 0 && lastReqCount == self->reqCount)
      waitStatus = flup_cond_wait(self->requestNeededEvent, self->requestLock, doTimedWait ? &deadline : NULL);
    firstCommand = false;
    
    if (lastReqCount == self->reqCount)
      goto there_are_no_request;
    
    enum stat_printer_status newStatus;
    switch (self->request) {
      case STAT_PRINTER_REQUEST_START:
        newStatus = STAT_PRINTER_STATUS_STARTED;
        doTimedWait = true;
        clock_gettime(CLOCK_REALTIME, &deadline);
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
    
there_are_no_request:
    recordData();
    
    if (!doTimedWait)
      continue;
    
    deadline.tv_nsec += intervalNanosecs;
    deadline.tv_sec  += intervalSecs + (deadline.tv_nsec / 1'000'000'000);
    deadline.tv_nsec %= 1'000'000'000;
    
    // Update display here
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    SDL_RenderFillRect(renderer, &(SDL_Rect) {
      .x = 0,
      .y = 0,
      .w = WIDTH,
      .h = HEIGHT
    });
    
    // Green: Target memory usage
    SDL_SetRenderDrawColor(renderer, 0x00, 0x88, 0x00, 0xFF);
    drawGraph(GRAPH_LINE, gcThresholdPointsWindow);
    
    // Red: Heap usage
    SDL_SetRenderDrawColor(renderer, 0x88, 0x00, 0x00, 0xFF);
    drawGraph(GRAPH_LINE, heapUsagePointsWindow);
    
    // Purple: average cycle times
    SDL_SetRenderDrawColor(renderer, 0x88, 0x00, 0x88, 0xFF);
    drawGraph(GRAPH_LINE, averageCycleTimeWindow);
    
    // Yello: proactive GC threshold
    SDL_SetRenderDrawColor(renderer, 0x88, 0x88, 0x00, 0xFF);
    drawGraph(GRAPH_LINE, proactiveGCThresholdPointsWindow);
    
    // Blue: live set size
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x88, 0xFF);
    drawGraph(GRAPH_LINE, liveSetSizeWindow);
    
    SDL_RenderPresent(renderer);
    
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      // Eat the events
    }
  }
  flup_mutex_unlock(self->requestLock);
  
  moving_window_free(gcThresholdPointsWindow);
  moving_window_free(heapUsagePointsWindow);
  
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
}

