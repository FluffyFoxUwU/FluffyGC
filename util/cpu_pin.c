#define _GNU_SOURCE

#include <sched.h>
#include <pthread.h>

#include "cpu_pin.h"

int util_cpu_pin_try_pin(pthread_t thrd, unsigned int cpuID) {
  cpu_set_t affinity;
  CPU_ZERO(&affinity);
  CPU_SET(cpuID, &affinity);
  
  return pthread_setaffinity_np(thrd, sizeof(affinity), &affinity) == 0;
}




