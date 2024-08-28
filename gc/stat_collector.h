#ifndef UWU_BEE0B45B_E914_422D_884A_063FD4B0D59C_UWU
#define UWU_BEE0B45B_E914_422D_884A_063FD4B0D59C_UWU

#include <stdatomic.h>

#include <flup/thread/thread.h>

#include "gc/gc.h"
#include "util/moving_window.h"

#define STAT_COLLECTOR_HZ 60
#define STAT_COLLECTOR_ALLOC_RATE_SAMPLES (STAT_COLLECTOR_HZ * 1)

struct stat_collector {
  struct gc_per_generation_state* gcState;
  flup_thread* thread;
  atomic_bool quitRequested;
  atomic_bool paused;
  
  // Alloc rate is in unit of bytes per 1/STAT_COLLECTOR_HZ
  struct moving_window* allocRateSamples;
  
  // These are updating regularly every STAT_COLLECTOR_HZ tick
  // Unit is bytes/sec
  atomic_size_t averageAllocRatePerSecond;
};

struct stat_collector* stat_collector_new(struct gc_per_generation_state* gcState);
void stat_collector_unpause(struct stat_collector* self);
void stat_collector_perform_shutdown(struct stat_collector* self);
void stat_collector_free(struct stat_collector* self);

#endif
