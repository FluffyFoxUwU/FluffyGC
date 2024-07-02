#ifndef UWU_CE719EB9_3F08_40E5_9881_38F580A8C20C_UWU
#define UWU_CE719EB9_3F08_40E5_9881_38F580A8C20C_UWU

#include <pthread.h>

// Return true on success, false on failure and negative on unsupported
int util_cpu_pin_try_pin(pthread_t thrd, unsigned int cpuID);

#endif
