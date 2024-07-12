#ifndef UWU_F702044F_AC05_44F6_BDBC_BD1DE8BBA7D0_UWU
#define UWU_F702044F_AC05_44F6_BDBC_BD1DE8BBA7D0_UWU

#include <stdatomic.h>

#include <flup/thread/thread.h>

// Checks the heap state 50 times a second
#define DRIVER_CHECK_RATE_HZ 50

struct gc_driver {
  struct gc_per_generation_state* gcState;
  flup_thread* driverThread;
  atomic_bool quitRequested;
};

struct gc_driver* gc_driver_new(struct gc_per_generation_state* gcState);
void gc_driver_free(struct gc_driver* self);

#endif
