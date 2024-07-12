#ifndef UWU_F702044F_AC05_44F6_BDBC_BD1DE8BBA7D0_UWU
#define UWU_F702044F_AC05_44F6_BDBC_BD1DE8BBA7D0_UWU

#include <stddef.h>
#include <stdatomic.h>

#include <flup/thread/thread.h>

// Checks the heap state 50 times a second
#define DRIVER_CHECK_RATE_HZ 50

// Two seconds worth of samples
#define DRIVER_ALLOC_RATE_SAMPLES (DRIVER_CHECK_RATE_HZ * 1)

struct gc_driver {
  struct gc_per_generation_state* gcState;
  flup_thread* driverThread;
  atomic_bool quitRequested;
  atomic_bool paused;
  
  // The time unit of the rate is is 1 / DRIVER_CHECK_RATE_HZ
  struct moving_window* runningSamplesOfAllocRates;
  size_t prevAllocBytes;
};

struct gc_driver* gc_driver_new(struct gc_per_generation_state* gcState);
void gc_driver_free(struct gc_driver* self);

void gc_driver_perform_shutdown(struct gc_driver* self);
void gc_driver_unpause(struct gc_driver* self);

#endif
